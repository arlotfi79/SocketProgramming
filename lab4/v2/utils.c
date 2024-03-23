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

