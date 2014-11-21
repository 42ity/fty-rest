#include "data.h"
#include "asset_types.h"
#include "measure_types.h"

#include <algorithm>

byte asset_manager::type_to_byte(std::string type) {
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    byte ret = asset_type::UNKNOWN;
    if(type == "datacenter") {
        ret = asset_type::DATACENTER;
    } else if(type == "room") {
        ret = asset_type::ROOM;
    } else if(type == "row") {
        ret = asset_type::ROW;
    } else if(type == "rack") {
        ret = asset_type::RACK;
    } else if(type == "group") {
        ret = asset_type::GROUP;
    } else if(type == "device") {
        ret = asset_type::DEVICE;
    }
    return ret;
}

std::string asset_manager::byte_to_type(byte type) {
    switch(type) {
        case asset_type::DATACENTER:
            return "datacenter";
        case asset_type::ROOM:
            return "room";
        case asset_type::ROW:
            return "row";
        case asset_type::RACK:
            return "rack";
        case asset_type::GROUP:
            return "group";
        case asset_type::DEVICE:
            return "device";
        default:
            return "unknown";
    }
}

std::string measures_manager::int_to_type(uint16_t i) {
    char buff[16];
    zmsg_t *req = common_msg_encode_get_measure_type_i(i);
    zmsg_t *rep = process_measures_meta(&req);
    common_msg_t *dta = NULL;
    if((rep != NULL) && ((dta = common_msg_decode(&rep)) != NULL) &&
       (common_msg_id(dta) == COMMON_MSG_RETURN_MEASURE_TYPE)) {
        std::string ret = common_msg_mt_name(dta);
        common_msg_destroy(&dta);
        return ret;
    } else {
        sprintf(buff, "%d", i);
        zmsg_destroy(&rep);
        common_msg_destroy(&dta);
        return buff;
    }
}

std::string measures_manager::int_to_type(std::string i) {
    errno = 0;
    uint16_t id = strtol(i.c_str(), NULL, 10);
    if(errno != 0) {
        return i;
    } else {
        return int_to_type(id);
    }
}

std::string measures_manager::int_to_subtype(uint16_t i, uint16_t tid) {
    char buff[16];
    zmsg_t *req = common_msg_encode_get_measure_subtype_i(i, tid);
    zmsg_t *rep = process_measures_meta(&req);
    common_msg_t *dta = NULL;
    if((rep != NULL) && ((dta = common_msg_decode(&rep)) != NULL) &&
       (common_msg_id(dta) == COMMON_MSG_RETURN_MEASURE_SUBTYPE)) {
        std::string ret = common_msg_mts_name(dta);
        common_msg_destroy(&dta);
        return ret;
    } else if((dta != NULL) && (common_msg_id(dta) == COMMON_MSG_FAIL)) {
        common_msg_print(dta); 
    }
    sprintf(buff, "%d", i);
    zmsg_destroy(&rep);
    common_msg_destroy(&dta);
    return buff;
}

std::string measures_manager::int_to_subtype(std::string i, std::string t) {
    errno = 0;
    uint16_t id =  strtol(i.c_str(), NULL, 10);
    uint16_t tid = strtol(t.c_str(), NULL, 10);
    if(errno != 0) {
        return i;
    } else {
        return int_to_subtype(id, tid);
    }
}
