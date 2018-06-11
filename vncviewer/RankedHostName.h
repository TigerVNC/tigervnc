#ifndef __RANKEDHOSTNAME_H__
#define __RANKEDHOSTNAME_H__

#include <string>

class RankedHostName {

protected:
    int rank;
    std::string name;
    bool pinned;

public:
    RankedHostName(int rank, std::string name, bool pinned);
    ~RankedHostName();

    int getRank();
    const std::string& getName();
    bool getPinned();

    void setRank(int&);

    bool operator< (const RankedHostName& h2) const;
    bool operator== (const RankedHostName& h2);
};

#endif // __RANKEDHOSTNAME_H__
