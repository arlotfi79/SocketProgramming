#ifndef UTILS_H
#define UTILS_H

struct forwardtab {
    unsigned long srcaddress;
    unsigned short srcport;
    unsigned long dstaddress;
    unsigned short dstport;
    unsigned short tunnelsport;
};

int find_available_entry(struct forwardtab tabentry[]);


#endif //UTILS_H
