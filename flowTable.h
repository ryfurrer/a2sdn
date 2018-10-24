#ifndef _Flow_Table_
#define  _Flow_Table_

#include <vector>

struct flow_entry {
    unsigned int srcIP_lo;
    unsigned int srcIP_hi;
    unsigned int destIP_lo;
    unsigned int destIP_hi;
    unsigned int actionType;
    unsigned int actionVal;
    unsigned int pri;
    unsigned int pktCount;
};
typedef std::vector<flow_entry> Flow_table;

#define DELIVER 0
#define FORWARD 1
#define DROP 2
#define MINPRI 4

#endif