#include "address.h"
#include "log.h"
#include <sstream>
#include <netdb.h>
#include <ifaddrs.h>
#include <stddef.h>


namespace bobliew {

int Address::getFamily() const{
    return getAddr()->sa_family;
}

socklen_t Address::getAddrLen() const {

}

std::ostream& Address::insert(std::ostream& os)const {

}

std::string toString() {

}



}
