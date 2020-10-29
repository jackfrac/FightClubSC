#pragma once
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/symbol.hpp>
#include <eosio/crypto.hpp>
#include <eosio/transaction.hpp>
#include <eosio/singleton.hpp>

#define MAINCONTRACT eosio::name("fightingclub")
#define HEROCONTRACT eosio::name("darkcountryh")
#define ATOMICCONTRACT eosio::name("atomicassets")
#define NESTCONTRACT eosio::name("nestplatform")


CONTRACT darkcountryf : public eosio::contract {
  public:
    using contract::contract;

    struct hero_s
    {
        std::string heroname;
        uint8_t rarity;
        uint32_t energy;
        uint32_t health;
    };

    struct fight_s
    {
        eosio::name username;
        std::string heroname;
        std::string attacktype;
        uint32_t damage;
        uint32_t energy;
        std::string blocktype;
        bool block;
    };

    ACTION setchance( uint64_t ahead, uint64_t agroin, uint64_t achest, uint64_t astomach, uint64_t alegs, 
                        uint64_t dhead, uint64_t dgroin, uint64_t dchest, uint64_t dstomach, uint64_t dlegs);

    //transfer action from atomic
    ACTION transferatom(eosio::name from, eosio::name to, std::vector<uint64_t> asset_ids, std::string memo);
    ACTION returnheroes(eosio::name username, uint64_t roomid);
    ACTION addheroes(eosio::name from, std::vector<uint64_t> asset_ids);

    ACTION deserialize(eosio::name username, hero_s hero1, hero_s hero2, hero_s hero3);

    //room actions
    ACTION createroom(eosio::name username, std::vector<uint64_t> heroes, uint64_t timestamp, uint8_t gametype);
    ACTION addtoroom(uint64_t roomid, eosio::name username, std::vector<uint64_t> heroes, uint64_t timestamp, uint8_t gametype);
    ACTION deleteroom(uint64_t roomid);
    ACTION cleanrooms();

    //fight actions
    ACTION turn(eosio::name username, std::string attack, std::string blocktype, bool specialatk);
    ACTION endturn(eosio::name username);
    ACTION fight(uint64_t roomid, eosio::name username1, std::string attacktype1, std::string block1, 
                eosio::name username2, std::string attacktype2, std::string block2);
    ACTION receiverand (uint64_t customer_id, const eosio::checksum256 &random_value);

    ACTION endgame(uint64_t roomid);

    //help action
    ACTION delgame(uint64_t id);
    ACTION deluser(eosio::name username);
    ACTION resetstats();

    //log actions
    ACTION logfight(eosio::name username1, std::string heroname1, uint64_t damage1, uint64_t randnumber1,
                    eosio::name username2, std::string heroname2, uint64_t damage2, uint64_t randnumber2);
    ACTION endgamelog(uint64_t roomid, eosio::name winner, eosio::name loser);

    private:

    TABLE chanceset
    {
        chanceset() {}
	    uint64_t ahead{0};
        uint64_t agroin{0};
        uint64_t achest{0};
        uint64_t astomach{0};
        uint64_t alegs{0};
        uint64_t dhead{0};
        uint64_t dgroin{0};
        uint64_t dchest{0};
        uint64_t dstomach{0};
        uint64_t dlegs{0};

        EOSLIB_SERIALIZE(chanceset, (ahead)(agroin)(achest)(astomach)(alegs)(dhead)(dgroin)(dchest)(dstomach)(dlegs))
    };
    typedef eosio::singleton< "chanceset"_n, chanceset> conf;
    chanceset _cstate;

    TABLE gameset
    {
        gameset() {}
        uint64_t roomid{0};
        uint64_t fightid{0};

        EOSLIB_SERIALIZE(gameset, (roomid)(fightid))
    };
    typedef eosio::singleton< "gameset"_n, gameset> gconf;
    gameset _gstate;


    TABLE gamesroom
    {
        uint64_t roomid{0};
        eosio::name username1;
        eosio::name username2;
        uint32_t round{0};
        std::vector<hero_s> heroes1;
        std::vector<hero_s> heroes2;
        std::vector<uint64_t> nftheroes1;
        std::vector<uint64_t> nftheroes2;
        uint64_t timestamp{0};
        uint8_t status{0};
        uint8_t gametype{0};

        uint64_t primary_key() const {return roomid;}
    };

    typedef eosio::multi_index<eosio::name("gamesrooms"), gamesroom> rooms;

    TABLE fightlog
    {
        uint64_t roomid;
        fight_s firstuser;
        fight_s seconduser;

        uint64_t primary_key() const {return roomid;}
    };

    typedef eosio::multi_index<eosio::name("fightlogs"), fightlog> fights;

    TABLE usergame
    {
        eosio::name username;
        uint64_t roomid{0};

        uint64_t primary_key() const {return username.value;}
    };

    typedef eosio::multi_index<eosio::name("usergames"), usergame> usersingames;

    TABLE hero
    {
        uint64_t heroid{0};

        uint64_t primary_key() const {return heroid;}
    };

    typedef eosio::multi_index<eosio::name("heroes"), hero> heroes;

    TABLE killtable
    {
        eosio::name username;
        uint64_t kills{0};
        uint64_t totaldamage{0};

        uint64_t primary_key() const {return username.value;}
    };

    typedef eosio::multi_index<eosio::name("killtables"), killtable> killtables;

    TABLE missround
    {
        eosio::name username;
        uint8_t rounds{0};

        uint64_t primary_key() const {return username.value;}
    };

    typedef eosio::multi_index<eosio::name("missrounds"), missround> missrounds;




    //////////////////////////////////////////////////////////
    //   atomic tables
    //////////////////////////////////////////////////////////
    typedef std::vector <int8_t>      INT8_VEC;
    typedef std::vector <int16_t>     INT16_VEC;
    typedef std::vector <int32_t>     INT32_VEC;
    typedef std::vector <int64_t>     INT64_VEC;
    typedef std::vector <uint8_t>     UINT8_VEC;
    typedef std::vector <uint16_t>    UINT16_VEC;
    typedef std::vector <uint32_t>    UINT32_VEC;
    typedef std::vector <uint64_t>    UINT64_VEC;
    typedef std::vector <float>       FLOAT_VEC;
    typedef std::vector <uint64_t>      uint64_t_VEC;
    typedef std::vector <std::string> STRING_VEC;
    
    typedef std::variant <\
            int8_t, int16_t, int32_t, int64_t, \
            uint8_t, uint16_t, uint32_t, uint64_t, \
            float, uint64_t, std::string, \
            INT8_VEC, INT16_VEC, INT32_VEC, INT64_VEC, \
            UINT8_VEC, UINT16_VEC, UINT32_VEC, UINT64_VEC, \
            FLOAT_VEC, uint64_t_VEC, STRING_VEC
        > ATOMIC_ATTRIBUTE;

    typedef std::map <std::string, ATOMIC_ATTRIBUTE> ATTRIBUTE_MAP;

    struct assets_s {
            uint64_t         asset_id;
            eosio::name             collection_name;
            eosio::name             schema_name;
            int32_t          template_id;
            eosio::name             ram_payer;
            std::vector <eosio::asset>   backed_tokens;
            std::vector <uint8_t> immutable_serialized_data;
            std::vector <uint8_t> mutable_serialized_data;

            uint64_t primary_key() const { return asset_id; };
        };

    typedef eosio::multi_index <eosio::name("assets"), assets_s> assets;

    /////////////////////////////////////////////////////////////////
    //  nest table
    /////////////////////////////////////////////////////////////////


    struct player_s
    {
      eosio::name               account;            //user wax account
      double                    points;             //user points
      std::string               data;               //user data on JSON
      double                    payout;             //user payouts
    };

    struct prize_s
    {
      uint8_t                   mode;               //prize mode: 0 - absolute numbers, 1 - percents
      std::vector<double>       values;             //users' prizes parts
    };

    TABLE lboard {
        uint64_t                  id;                 //nestplatform id on table
        eosio::name               owner;              //owner wax account name
        std::string               boardname;          //name of current board
        uint64_t                  gameid;             //game's id
        double                    pot;                //total pot on current nestplatform
        std::vector<player_s>     players;            //array of players
        prize_s                   prize;              //array and type of prizes

        uint64_t primary_key() const { return id; }
    };
    typedef eosio::multi_index<eosio::name("lboards"), lboard> lboards;

    public:
    uint64_t getroomid();
    std::string getheroname(std::vector<hero_s> heroes);
    uint64_t setdamage(uint64_t randnumb, std::string attacktype, std::string blocktype, bool specialatk, uint8_t rarity);
    uint8_t findhero(std::vector<hero_s> herovec);
    int finder(std::vector<player_s> players, eosio::name username);
    int setheropoints(std::vector<hero_s> heroes_ids);
};