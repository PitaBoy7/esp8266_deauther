/*
   Copyright (c) 2020 Stefan Kremser (@Spacehuhn)
   This software is licensed under the MIT License. See the license file for details.
   Source: github.com/spacehuhn/esp8266_deauther
 */

#include "alias.h"
#include "strh.h"
#include "mac.h"
#include "debug.h"
#include "eeprom.h"
#include "vendor.h"

typedef struct alias_t {
    uint8_t mac[6];
    char    name[MAX_ALIAS_LEN];
} alias_t;

typedef struct alias_list_t {
    uint32_t magic_num;
    alias_t  data[MAX_ALIAS_NUM];
    int      size;
} alias_list_t;

namespace alias {
    alias_list_t list;

    void clear() {
        list.size      = 0;
        list.magic_num = ALIAS_MAGIC_NUM;

        add(mac::BROADCAST, "broadcast");
    }

    void load() {
        eeprom::getObject(ALIAS_ADDR, list);

        if ((list.magic_num != ALIAS_MAGIC_NUM) || (list.size > MAX_ALIAS_NUM)) {
            clear();
            // debugln("Resetted MAC alias list");
        } else {
            // debugln("Loaded MAC alias list");
        }
    }

    void save() {
        eeprom::saveObject(ALIAS_ADDR, list);
    }

    int search(const uint8_t* mac) {
        if (mac) {
            for (unsigned int i = 0; i<list.size && i<MAX_ALIAS_NUM; ++i) {
                if (memcmp(list.data[i].mac, mac, 6) == 0) {
                    return i;
                }
            }
        }
        return -1;
    }

    int search(const String& name) {
        for (unsigned int i = 0; i<list.size && i<MAX_ALIAS_NUM; ++i) {
            if (strncmp(list.data[i].name, name.c_str(), MAX_ALIAS_LEN) == 0) {
                return i;
            }
        }
        return -1;
    }

    bool add(const uint8_t* mac, const String& name) {
        if ((list.size >= MAX_ALIAS_NUM) ||
            (search(mac) > 0) ||
            (search(name) > 0)) return false;

        alias_t* a = &list.data[list.size];
        memcpy(a->mac, mac, 6);
        strncpy(a->name, name.c_str(), MAX_ALIAS_LEN);
        ++list.size;
        save();

        return true;
    }

    String get(const uint8_t* mac) {
        int id = search(mac);

        if ((id < 0) || (id > list.size)) return strh::mac(mac);

        return getName(id);
    }

    bool resolve(const String& name, uint8_t* buffer) {
        int id = search(name);

        if ((id < 0) || (id > list.size)) return false;

        memcpy(buffer, list.data[id].mac, 6);
        return true;
    }

    String getName(int id) {
        if ((id < 0) || (id > list.size)) return String();

        String res;

        for (unsigned int i = 0; i<MAX_ALIAS_LEN && list.data[id].name[i] != '\0'; ++i) {
            res += char(list.data[id].name[i]);
        }

        return res;
    }

    bool remove(int id) {
        if ((id < 0) || (id > list.size)) return false;

        for (unsigned int i = id; i<list.size && i<MAX_ALIAS_NUM; ++i) {
            alias_t* c = &list.data[i];
            alias_t* n = &list.data[i+1];
            memcpy(c->mac, n->mac, 6);
            strncpy(c->name, n->name, MAX_ALIAS_LEN);
        }
        --list.size;
        save();
        return true;
    }

    bool remove(const uint8_t* mac) {
        return remove(search(mac));
    }

    bool remove(const String& name) {
        return remove(search(name));
    }

    void print() {
        debug("MAC Alias List: ");
        debugln(list.size);

        debug(strh::right(3, "ID"));
        debug(' ');
        debug(strh::left(MAX_ALIAS_LEN, "Name"));
        debug(' ');
        debug(strh::left(17, "MAC-Address"));
        debug(' ');
        debug(strh::left(8, "Vendor"));
        debugln();
        debugln("===========================================");

        for (unsigned int i = 0; i<list.size && i<MAX_ALIAS_NUM; ++i) {
            debug(strh::right(3, String(i)));
            debug(' ');
            debug(strh::left(MAX_ALIAS_LEN, getName(i)));
            debug(' ');
            debug(strh::left(17, strh::mac(list.data[i].mac)));
            debug(' ');
            debug(strh::left(8, vendor::search(list.data[i].mac)));
            debugln();
        }
        debugln("===========================================");

        debugln();
    }
}