#include "utils.h"
#include "constants.h"

int find_available_entry(struct forwardtab tabentry[]) {
    int tunnel_index;
    for (tunnel_index = 0; tunnel_index < MAX_CLIENTS; tunnel_index++) {
        if (tabentry[tunnel_index].srcaddress == 0) {
            break;
        }
    }
    return (tunnel_index < MAX_CLIENTS) ? tunnel_index : -1;
}

void free_session_entry(struct forwardtab tabentry[], int index) {
    tabentry[index].srcaddress = 0;
    tabentry[index].srcport = 0;
    tabentry[index].dstaddress = 0;
    tabentry[index].dstport = 0;
    tabentry[index].tunnelsport = 0;
}

