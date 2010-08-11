#include <sys/param.h>
#include <sys/mount.h>
struct null_args {
    char        *target;    /* Target of loopback  */
};


int main() {
    struct null_args args;
    args.target = "/y";
    mount("loopback", "/x", 0, &args);
}
