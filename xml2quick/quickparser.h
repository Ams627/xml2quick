#pragma once
#include <ams/rjis/rjisdate.h>
#include <ams/rjis/stationcode.h>
#include <ams/rjis/tlvutils.h>

struct QuickInfo
{
    int serialNumber_;
    UNLC origin_;               
    UNLC destination_;
    RJISDate::Range daterange_;
    std::string route_;
    std::string ticketCode_;
    std::string restrictionCode_;
    std::string statusCode_;
    int adultfare_;
    int flag_;
    int crossLondon_;
    std::string timeband_;
    std::string dateband_;
    int orientation_;
};

struct TimeRange
{
    uint32_t startseconds_; // seconds since midnight
    uint32_t endseconds_;   // seconds since midnight
};

struct TimeBandInfo
{
    std::string name;
    std::vector<TimeRange> timeranges; // time of day range 
};

struct QuickKey
{
    UNLC origin;
    std::string tvm;
    bool operator<(const QuickKey& other) const { return std::forward_as_tuple(origin, tvm) < std::forward_as_tuple(other.origin, other.tvm); }
};


class Quickparser
{
    std::string filename_;
    std::map<QuickKey, std::vector<QuickInfo>> outputmap_;      // key is (nlc, deviceid) combo
    std::vector<TimeBandInfo> timebands_;                       // timebands is global but appended to the end of each QUICK_SE file

public:
    Quickparser(std::string filename) : filename_(filename) {}
    void Parse();
    void WriteTLV(int version);
    virtual ~Quickparser(){}
};
