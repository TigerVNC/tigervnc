#include "RankedHostName.h"

RankedHostName::RankedHostName(int rank, std::string name, bool pinned)
: rank(rank)
, name(name)
, pinned(pinned) {

}

RankedHostName::~RankedHostName() {
}

int RankedHostName::getRank() {
    return rank;
}

const std::string& RankedHostName::getName() {
    return name;
}

bool RankedHostName::getPinned() {
    return pinned;
}

void RankedHostName::setRank(int &rank) {
    this->rank = rank;
}

bool RankedHostName::operator< (const RankedHostName& h2) {
    if (pinned == h2.pinned)
        return rank < h2.rank;
    else if (pinned)
        return true;
    return false;
}

bool RankedHostName::operator== (const RankedHostName& h2) {
    return name == h2.name;
}
