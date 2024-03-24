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
void free_session_entry(struct forwardtab tabentry[], int index);

#endif //UTILS_H
