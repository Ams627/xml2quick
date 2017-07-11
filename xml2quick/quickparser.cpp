#include "stdafx.h"
#include "quickparser.h"
#include "tixmlutil.h"
#include "TLVTypes.h"


// Vente ID_QCKSEL_CODE(6; 6516) uint 1
// Vente ID_QCKSEL_END_DATE(6; 6517) date 15 / 04 / 2047 02:42 : 07
// Vente ID_QCKSEL_START_DATE(6; 6518) date 02 / 01 / 2014 00 : 00 : 00
// Vente ID_QCKSEL_ORIGIN(6; 6520) string "9039"
// Vente ID_QCKSEL_DESTINATION(6; 6521) string "9328"
// Vente ID_QCKSEL_ROUTE(6; 6522) string "00000"
// Vente ID_QCKSEL_TICKET(6; 6523) string "SDS"
// Vente ID_QCKSEL_RESTRICTION(6; 6524) string "  "
// Vente ID_QCKSEL_ADULT_FARE(6; 6526) uint 1730
// Vente ID_QCKSEL_CROSS_LONDON_IND(6; 6528) uint 0
// Vente ID_QCKSEL_FLAG(6; 6529) uint 1
// Vente ID_QCKSEL_STATUS(6; 6530) string "000"
// Vente ID_QCKSEL_ORIENTATION(6; 6538) uint 0
// Vente ID_QCKSEL_DATEBAND_NAME(6; 6539) string "YYYYYNN"
// Vente ID_QCKSEL_TIMEBAND_NAME(6; 6541) string "Slot 11"



//<QuickSelects>
//<device nlc="9039" tvm="1200">
//    <quick dest="9415" route="00000" tcode="7DF" sdate="2014-10-9" edate="2015-1-1" dband="YYYYYYNN" tband="08:00-09:00" ori="left">
//    <quick dest="9415" route="00000" tcode="7DF" sdate="2014-10-9" edate="2015-1-1" dband="YYYYYYNN" tband="08:00-09:00" ori="left">
//    <timeband name="fred" start="0915" end="0930">
//</device>
//<QuickSelects>

enum class TLVQuickConstants
{
    ID_QCKSEL_NUMERO_VERSION = 6513,
    ID_QCKSEL_IAP = 6514,
    ID_QCKSEL_PRODUCT = 6515,
    ID_QCKSEL_CODE = 6516,
    ID_QCKSEL_END_DATE = 6517,
    ID_QCKSEL_START_DATE = 6518,
    ID_QCKSEL_ORIGIN = 6520,
    ID_QCKSEL_DESTINATION = 6521,
    ID_QCKSEL_ROUTE = 6522,
    ID_QCKSEL_TICKET = 6523,
    ID_QCKSEL_RESTRICTION = 6524,
    ID_QCKSEL_ADULT_FARE = 6526,
    ID_QCKSEL_CROSS_LONDON_IND = 6528,
    ID_QCKSEL_FLAG = 6529,
    ID_QCKSEL_STATUS = 6530,
    ID_QCKSEL_ORIENTATION = 6538,
    ID_QCKSEL_DATEBAND_NAME = 6539,
    ID_QCKSEL_TIMEBAND_NAME = 6541,
    ID_QCKSEL_TIMEBAND_TABLE = 6535,
    ID_QCKSEL_TIMEBAND_START = 6536,
    ID_QCKSEL_TIMEBAND_END = 6537,
    ID_QCKSEL_TIMEBAND_ARRAY = 6542
};


namespace {
    // check the string length in attribute name 'name' at linenumber 'linenumber'
    void CheckStringLengthThrow(std::string name, std::string str, size_t len, int linenumber)
    {
        if (str.length() != len)
        {
            throw QException(linenumber, name + " must be " + std::to_string(len) + " characters.");
        }
    }

    void CheckISO8601Date(RJISDate::Date& dout, const std::string name, const std::string str, int linenumber)
    {
        if (str.length() != 10 || str[4] != '-' || str[7] != '-' ||
            !isdigit(str[0]) || !isdigit(str[1]) || !isdigit(str[2]) || !isdigit(str[3]) ||
            !isdigit(str[5]) || !isdigit(str[6]) || !isdigit(str[8]) || !isdigit(str[9]))
        {
            throw QException(linenumber, "invalid date: " + name + " must be a date like this: 2014-02-16.");
        }
        int y, m, d;
        try
        {
            y = std::stoi(str);
            m = std::stoi(str.substr(5));
            d = std::stoi(str.substr(8));
        }
        catch (std::exception&)
        {
            throw  QException(linenumber, "invalid date: " + name + " must be " + " a date like this: 2014-02-16.");
        }
        if (!RJISDate::IsDateValid(y, m, d))
        {
            throw QException("invalid date '" + str + "' for attribute " + name);
        }
        dout = RJISDate::Date(y, m, d);
    }

    void CheckTime(uint32_t & seconds, const std::string attributename, const std::string strtime, int linenumber)
    {
        if (strtime.length() != 8 || strtime[2] != ':' || strtime[5] != ':' ||
            !isdigit(strtime[0]) || !isdigit(strtime[1]) || !isdigit(strtime[3]) || !isdigit(strtime[4]) ||
            !isdigit(strtime[6]) || !isdigit(strtime[7]))
        {
            throw QException(linenumber, "invalid time: " + attributename + " must be " + " a time like this: '21:39:19'.");
        }
        int h, m, s;
        try
        {
            h = std::stoi(strtime);
            m = std::stoi(strtime.substr(3));
            s = std::stoi(strtime.substr(6));
        }
        catch (std::exception&)
        {
            throw QException(linenumber, "invalid time: " + attributename + " must be " + " a time like this: '21:39:19'.");
        }
        seconds = 3600 * h + 60 * m + s;
    }

    // write the info for a single quick select
    size_t WriteTLVQuickEntry(char*buf, const QuickInfo& qi)
    {
        TLVInt serialNumber(qi.serialNumber_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_CODE));
        TLVDateRange daterange(qi.daterange_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_END_DATE));
        TLVString origin(qi.origin_.GetString(), static_cast<int>(TLVQuickConstants::ID_QCKSEL_ORIGIN));
        TLVString destination(qi.destination_.GetString(), static_cast<int>(TLVQuickConstants::ID_QCKSEL_DESTINATION));
        TLVString route(qi.route_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_ROUTE));
        TLVString ticketCode(qi.ticketCode_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_TICKET));
        TLVString restrictionCode(qi.restrictionCode_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_RESTRICTION));
        TLVInt adultfare(qi.adultfare_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_ADULT_FARE));
        TLVInt crossLondon(qi.crossLondon_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_CROSS_LONDON_IND));
        TLVInt flag(qi.flag_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_FLAG));
        TLVString status(qi.statusCode_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_STATUS));
        TLVInt orientation(qi.orientation_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_ORIENTATION));
        TLVString dateband(qi.dateband_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_DATEBAND_NAME));
        TLVString timeband(qi.timeband_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_TIMEBAND_NAME));

        size_t pos = 0;
        pos += serialNumber.Write(buf + pos);
        pos += daterange.Write(buf + pos);
        pos += origin.Write(buf + pos);
        pos += destination.Write(buf + pos);
        pos += route.Write(buf + pos);
        pos += ticketCode.Write(buf + pos);
        pos += restrictionCode.Write(buf + pos);
        pos += adultfare.Write(buf + pos);
        pos += crossLondon.Write(buf + pos);
        pos += flag.Write(buf + pos);
        pos += status.Write(buf + pos);
        pos += orientation.Write(buf + pos);
        pos += dateband.Write(buf + pos);
        pos += timeband.Write(buf + pos);
        return pos;
    }


    template <class Cont> size_t WriteTimebandArray(char* buf, const Cont& cont)
    {
        static const int typecodeDate = 4;
        size_t pos = 0;

        std::array<BYTE, 256> tempdatabuf;

        for (auto p : cont)
        {
            TLVInt startime(p.startseconds_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_TIMEBAND_START), typecodeDate);
            TLVInt endtime(p.endseconds_, static_cast<int>(TLVQuickConstants::ID_QCKSEL_TIMEBAND_END), typecodeDate);

            // write the data to a temporary buffer:
            size_t tempoffset = startime.Write(tempdatabuf.data());
            tempoffset += endtime.Write(tempdatabuf.data() + tempoffset);

            // write the length of the data to the passed in buffer
            uint32_t datalength; // variable to hold the converted length
            size_t lenlen = TLVUtils::Convert(datalength, tempoffset);
            PP(lenlen);
            PPX(datalength);
            TLVUtils::EnBuffer((BYTE*)buf, datalength, lenlen);
            for (size_t i = 0; i < tempoffset; ++i)
            {
                buf[pos + lenlen + i] = tempdatabuf[i];
            }
            pos = pos + lenlen + tempoffset;
        }
        return pos;
    }

    // write the info for a quick select timeband
    size_t WriteTLVTimebandEntry(char* buf,  const TimeBandInfo &tb)
    {
        static const int typecodeDate = 3;
        size_t pos = 0;

        TLVString timebandname(tb.name, static_cast<int>(TLVQuickConstants::ID_QCKSEL_TIMEBAND_NAME));
        pos = timebandname.Write(buf);

        // write the timeband array to the BUFFER, in order to get its 
        // length - once we have got its length we will need to write it again!
        size_t length = WriteTimebandArray(buf + pos, tb.timeranges);

        // write the length of the timeband array
        TLVArrayHeader lengthForTLV(length, static_cast<int>(TLVQuickConstants::ID_QCKSEL_TIMEBAND_ARRAY));
        pos += lengthForTLV.Write(buf + pos);
        // write the timeband array for the second time:
        pos = WriteTimebandArray(buf + pos, tb.timeranges);

        return pos;

    }

    template <class Cont> size_t WriteQuicksForOneTVM(std::ofstream& ostr, bool write, const Cont& quickselects)
    {
        size_t result = 0;
        std::array<char, 1024> tlvBuffer;

        for (auto p : quickselects)
        {
            size_t pos = WriteTLVQuickEntry(tlvBuffer.data(), p);
            uint32_t datalength; // variable to hold the converted length
            size_t lenlen = TLVUtils::Convert(datalength, pos);
            PP(lenlen);
            PPX(datalength);
            std::array<BYTE, 8> lenbuf;
            TLVUtils::EnBuffer(lenbuf.data(), datalength, lenlen);
            result = result + lenlen + pos;
            if (write)
            {
                ostr.write((char*)lenbuf.data(), lenlen);
                ostr.write(tlvBuffer.data(), pos);
            }
        }
        return result;
    }


    template <class Cont> size_t WriteBandsForOneTVM(std::ofstream& ostr, bool write, const Cont& timebands)
    {
        std::array<char, 32768> tlvBuffer;
        std::array<BYTE, 8> lenbuf;
        size_t result = 0;

        // each timeband is a name and a vector of (startsecond, endsecond) time ranges
        for (auto p : timebands)
        {
            // write the timeband to a temporary buffer:
            size_t pos = WriteTLVTimebandEntry(tlvBuffer.data(), p);
            // write the length of the timeband to a temporary buffer:
            uint32_t datalength; // variable to hold the converted length
            size_t lenlen = TLVUtils::Convert(datalength, pos);
            TLVUtils::EnBuffer(lenbuf.data(), datalength, lenlen);
            result = result + lenlen + pos;
            if (write)
            {
                ostr.write((char*)lenbuf.data(), lenlen);
                ostr.write(tlvBuffer.data(), pos);
            }
        }
        return result;
    }
}


// write TLV files (one per TVM) given fares version number:
void Quickparser::WriteTLV(int version)
{
    static const std::array<char, 8> tlvheader = { 'T', 'L', 't', 'V', '0', '1', '0', '0' };
    std::array<char, 256> buf;

    // for each tvm in the output map:
    for (auto origin : outputmap_)
    {
        assert(origin.first.origin.GetString().length() == 4);
        // filename is QUICK_SE plus the NLC code (always 4 characters) then the tvm name if there are multiple
        // tvms at a station with different quick selects
        std::string filename = "QUICK_SE." + origin.first.origin.GetString() + "." + origin.first.tvm;
        std::ofstream ofsTLV(filename, std::ios::binary);
        if (!ofsTLV)
        {
            throw QException("cannot write output file " + filename);
        }
        // write the TLV header "TLtV0100":
        ofsTLV.write(tlvheader.data(), tlvheader.size());

        // write first two non-array items to each tlv quick select file:
        TLVInt tlvFaresVersion(version, static_cast<int>(TLVQuickConstants::ID_QCKSEL_NUMERO_VERSION));
        TLVString iap(origin.first.tvm, static_cast<int>(TLVQuickConstants::ID_QCKSEL_IAP));
        size_t pos = tlvFaresVersion.Write(buf.data());
        pos += iap.Write(buf.data() + pos);

        // get the length of quick select array data to write:
        size_t quickarrayLength = WriteQuicksForOneTVM(ofsTLV, false, origin.second);
        PP(quickarrayLength);
        TLVArrayHeader tlvarrayLength(quickarrayLength, static_cast<int>(TLVQuickConstants::ID_QCKSEL_PRODUCT));
        pos += tlvarrayLength.Write(buf.data() + pos);
        ofsTLV.write(buf.data(), pos);
        ofsTLV.flush(); // AMS DEBUG
        // actually write the quickselects this time:
        WriteQuicksForOneTVM(ofsTLV, true, origin.second);

        pos = 0;
        // get the length of the timeband array but DON'T WRITE IT:
        size_t bandarrayLength = WriteBandsForOneTVM(ofsTLV, false, timebands_);
        PP(bandarrayLength);
        // write the array header and the length of the timeband array:
        TLVArrayHeader tlvbandarrayLength(bandarrayLength, static_cast<int>(TLVQuickConstants::ID_QCKSEL_TIMEBAND_TABLE));
        pos += tlvbandarrayLength.Write(buf.data() + pos);
        ofsTLV.write(buf.data(), pos);
        ofsTLV.flush(); // AMS DEBUG
        // actually write the timebands this time:
        WriteBandsForOneTVM(ofsTLV, true, timebands_);
        ofsTLV.flush(); // AMS DEBUG
    }
}

void Quickparser::Parse()
{
    int currentQSelCode = 0;
    int linenumber = 0;
    TiXmlDocument doc(filename_);
    bool result = doc.LoadFile();
    TiXmlElement* pRootElement = doc.RootElement();
    std::string rootElementName = pRootElement->ValueStr();
    if (rootElementName != "quickselects")
    {
        std::cerr << "root element expected: quickselects\n";
    }
    for (const TiXmlElement* pDevice = pRootElement->FirstChildElement("device");
        pDevice; pDevice = pDevice->NextSiblingElement("device"))
    {
        std::string origin = TixmlUtil::GetStringAttribute(pDevice, "nlc");
        std::string device = TixmlUtil::GetStringAttribute(pDevice, "tvm");

        QuickKey key;
        key.origin = origin;
        key.tvm = device;

        for (const TiXmlElement* pQuick = pDevice->FirstChildElement("quick"); pQuick; pQuick = pQuick->NextSiblingElement("quick"))
        {
            try
            {
                QuickInfo qi;
                qi.origin_ = origin;
                bool got;
                // if no quick select code supplied, then use an incrementing counter:
                qi.serialNumber_ = TixmlUtil::GetOptIntAttribute(got, pQuick, "code");
                if (!got)
                {
                    qi.serialNumber_ = currentQSelCode++;
                }
                qi.route_ = TixmlUtil::GetStringAttributeLen(pQuick, "route", 5);
                qi.ticketCode_ = TixmlUtil::GetStringAttributeLen(pQuick, "tcode", 3);
                qi.restrictionCode_ = TixmlUtil::GetStringAttributeLen(pQuick, "rescode", 2);
                qi.statusCode_ = TixmlUtil::GetStringAttributeLen(pQuick, "status", 3);

                qi.dateband_ = TixmlUtil::GetStringAttribute(pQuick, "dband");
                qi.timeband_ = TixmlUtil::GetStringAttribute(pQuick, "tband");
                qi.crossLondon_ = TixmlUtil::GetZeroOneAttribute(pQuick, "crosslon");
                qi.flag_ = TixmlUtil::GetZeroOneAttribute(pQuick, "flag");
                qi.orientation_ = TixmlUtil::GetZeroOneAttribute(pQuick, "orient");
                qi.adultfare_ = TixmlUtil::GetIntAttribute(pQuick, "afare");

                qi.destination_ = TixmlUtil::GetStringAttributeLen(pQuick, "dest", 4);

                std::string startdate = TixmlUtil::GetStringAttribute(pQuick, "sdate");
                std::string enddate = TixmlUtil::GetStringAttribute(pQuick, "edate");
                RJISDate::Date dend, dstart;
                CheckISO8601Date(dend, "edate", enddate, pQuick->Row());
                CheckISO8601Date(dstart, "sdate", startdate, pQuick->Row());
                qi.daterange_ = RJISDate::Range(dend, dstart);

                outputmap_[key].push_back(qi);
            }
            catch (QException& e)
            {
                std::cerr << e.what() << std::endl;
            }
            linenumber++;
        }

        // <timeband name="fred">
        for (const TiXmlElement* pTimeband = pRootElement->FirstChildElement("timeband");
            pTimeband;
            pTimeband = pTimeband->NextSiblingElement("timeband"))
        {
            TimeBandInfo tbinfo;
            tbinfo.name = TixmlUtil::GetStringAttribute(pTimeband, "name");

            // example: <timerange start="09:30:21" end="23:44:10"/>
            for (const TiXmlElement* pTimeRange = pTimeband->FirstChildElement("timerange");
                pTimeRange;
                pTimeRange = pTimeRange->NextSiblingElement("timerange"))
            {
                try
                {
                    int linenumber = pTimeRange->Row();
                    std::string starttime = TixmlUtil::GetStringAttribute(pTimeRange, "start");
                    std::string endtime = TixmlUtil::GetStringAttribute(pTimeRange, "end");
                    TimeRange timerange;
                    CheckTime(timerange.startseconds_, "start", starttime, linenumber);
                    CheckTime(timerange.endseconds_, "end", endtime, linenumber);
                    tbinfo.timeranges.push_back(timerange);
                }
                catch (QException& e)
                {
                    std::cerr << e.what();
                }
            }
            timebands_.push_back(tbinfo);
        }
    }
}
