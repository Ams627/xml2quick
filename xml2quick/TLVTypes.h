#pragma once
#include <ams/rjis/tlvutils.h>

static const uint8_t VenteModule = 6;

union UTLV32
{
    int i;
    uint32_t u;
#ifdef BIGENDIAN
    struct  
    {
        uint8_t b3;
        uint8_t b2;
        uint8_t b1;
        uint8_t b0;
    } bytes;
#else
    struct
    {
        uint8_t b0;
        uint8_t b1;
        uint8_t b2;
        uint8_t b3;
    } bytes;
    UTLV32() {}
    UTLV32(int i) : i(i) {}
    UTLV32(uint32_t u) : u(u) {}
#endif
};

#if 0
union UTLV64
{
    int i;
    uint32_t u;
#ifdef BIGENDIAN
    struct
    {
        uint8_t b7;
        uint8_t b6;
        uint8_t b5;
        uint8_t b4;
        uint8_t b3;
        uint8_t b2;
        uint8_t b1;
        uint8_t b0;
    } bytes;
#else
    struct
    {
        uint8_t b0;
        uint8_t b1;
        uint8_t b2;
        uint8_t b3;
        uint8_t b4;
        uint8_t b5;
        uint8_t b6;
        uint8_t b7;
    } bytes;
    UTLV64() {}
    UTLV64(uint64_t i) : i(i) {}
    UTLV64(uint32_t u) : u(u) {}
#endif
};
#endif

class TLVString
{
    const std::string s_;
    UTLV32 tag_;
public:
    TLVString(const std::string s, int tag) : s_(s), tag_(tag) {}
    TLVString(const char* s, int tag) : s_(s), tag_(tag) {}
    template<size_t N> TLVString(std::array<char, N> arr, int tag) : s_(std::cbegin(arr), std::cend(arr)), tag_(tag) {}

    size_t Write(void*buf, bool setmod = false, bool settag = false)
    {
        size_t written = 0;
        uint8_t module = VenteModule;
        if (setmod)
        {
            module |= 0x80;
        }
        if (settag)
        {
            tag_.bytes.b1 |= 0x80;
        }
        BYTE *pBuffer = reinterpret_cast<BYTE*>(buf);
        *pBuffer++ = module;
        *pBuffer++ = tag_.bytes.b1 | 0x40;
        *pBuffer++ = tag_.bytes.b0;
        uint32_t tlvLength;
        size_t lengthlength = TLVUtils::Convert(tlvLength, s_.length() + 1);
        TLVUtils::EnBuffer(pBuffer, tlvLength, lengthlength);
        pBuffer += lengthlength;

        // add the type indicator to the buffer: STRING = 2
        *pBuffer++ = 0x02;

        // finally add the string itself to the buffer
        for (auto c : s_)
        {
            *pBuffer++ = c;
        }
        written = 4 + lengthlength + s_.length();
        return written;
    }
};

class TLVInt
{
    uint32_t value_;
    UTLV32 tag_;
    int typecode_;
public:
    TLVInt(uint32_t value, int tag, int typecode = 1) : value_(value), tag_(tag), typecode_(typecode) {}
    size_t Write(void*buf, bool setmod = false, bool settag = false)
    {
        size_t written;
        uint8_t module = VenteModule;
        if (setmod)
        {
            module |= 0x80;
        }
        if (settag)
        {
            tag_.bytes.b1 |= 0x80;
        }
        BYTE *pBuffer = reinterpret_cast<BYTE*>(buf);
        *pBuffer++ = module;
        *pBuffer++ = tag_.bytes.b1 | 0x40;
        *pBuffer++ = tag_.bytes.b0;

        uint64_t outputint;
        size_t intlength = TLVUtils::Convert(outputint, value_);
        *pBuffer++ = intlength + 1;
        // type code for integers:
        *pBuffer++ = typecode_;

        TLVUtils::EnBuffer(pBuffer, outputint, intlength);
        written = 5 + intlength;
        return written;
    }
};

class TLVInt64
{
    uint64_t value_;
    UTLV32 tag_;
public:
    TLVInt64(uint64_t value, int tag) : value_(value), tag_(tag) {}
    size_t Write(void*buf, int typecode = 1, bool setmod = false, bool settag = false)
    {
        size_t written;
        uint8_t module = VenteModule;
        if (setmod)
        {
            module |= 0x80;
        }
        if (settag)
        {
            tag_.bytes.b1 |= 0x80;
        }
        BYTE *pBuffer = reinterpret_cast<BYTE*>(buf);
        *pBuffer++ = module;
        *pBuffer++ = tag_.bytes.b1 | 0x40;
        *pBuffer++ = tag_.bytes.b0;

        uint64_t outputint;
        size_t intlength = TLVUtils::Convert(outputint, value_);
        *pBuffer++ = intlength + 1;
        // type code for integers:
        *pBuffer++ = typecode;
        TLVUtils::EnBuffer(pBuffer, outputint, intlength);
        written = 5 + intlength;
        return written;
    }
};

class TLVDateRange
{
    UTLV32 tag_;
    RJISDate::Range range_;
public:
    TLVDateRange(RJISDate::Range range, int tag) : range_(range), tag_(tag) {}
    size_t Write(void*buf, bool setmod = false, bool settag = false)
    {
        static RJISDate::Date endOfRange(2999, 12, 31);
        size_t written = 0;
        uint8_t module = VenteModule;
        if (setmod)
        {
            module |= 0x80;
        }
        if (settag)
        {
            tag_.bytes.b1 |= 0x80;
        }
        BYTE *pBuffer = reinterpret_cast<BYTE*>(buf);

        int64_t secondsEnd = 1LL * range_.GetEndDate().GetDays() * 3600 * 24;
        if (range_.GetEndDate() == endOfRange)
        {
            // adjust to 23:59 if on 31-12-2999 as this is what the french do! :)
            secondsEnd += 3600 * 24 - 1;
        }
        uint64_t secondsStart = 1LL * range_.GetStartDate().GetDays() * 3600 * 24;


        *pBuffer++ = module;
        *pBuffer++ = tag_.bytes.b1 | 0x40;
        *pBuffer++ = tag_.bytes.b0;

        uint64_t outputint;
        size_t intlength = TLVUtils::Convert(outputint, secondsEnd);
        *pBuffer++ = intlength + 1;
        // type code for dates:
        *pBuffer++ = 4;
        // write the start date itself into the buffer:
        TLVUtils::EnBuffer(pBuffer, outputint, intlength);
        pBuffer += intlength;

        written = intlength + 5;

        // use the input tag + 1 for the start date:
        tag_.u++;
        *pBuffer++ = module;
        *pBuffer++ = tag_.bytes.b1 | 0x40;
        *pBuffer++ = tag_.bytes.b0;

        intlength = TLVUtils::Convert(outputint, secondsStart);
        *pBuffer++ = intlength + 1;
        // type code for dates:
        *pBuffer++ = 4;
        // write the start date itself into the buffer:
        TLVUtils::EnBuffer(pBuffer, outputint, intlength);

        written = written + intlength + 5;

        return written;
    }
};

class TLVArrayHeader
{
    UTLV32 tag_;
    uint32_t value_;
public:
    TLVArrayHeader(uint32_t totalLength, uint32_t tag) : tag_(tag), value_(totalLength) {}
    size_t Write(void*buf)
    {
        BYTE *pBuffer = reinterpret_cast<BYTE*>(buf);
        *pBuffer++ = VenteModule | 0x80;
        *pBuffer++ = tag_.bytes.b1 | 0xc0;
        *pBuffer++ = tag_.bytes.b0;

        uint64_t outputint;
        size_t intlength = TLVUtils::Convert(outputint, value_);
        TLVUtils::EnBuffer(pBuffer, outputint, intlength);
        return intlength + 3;
    }

};