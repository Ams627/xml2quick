#include "stdafx.h"
#include "quickparser.h"

int main(int argc, char**argv)
{
    try
    {
        if (argc < 2)
        {
            throw QException("You must supply an xml file as an argument.");
        }
        std::string filename = argv[1];
        Quickparser qparser(filename);
        qparser.Parse();
        qparser.WriteTLV(22);
    }
    catch (QException& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
