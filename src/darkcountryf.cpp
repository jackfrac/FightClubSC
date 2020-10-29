#include <darkcountryf.hpp>

//TODO kill counter && total damage add 


ACTION darkcountryf::setchance( uint64_t ahead, uint64_t agroin, uint64_t achest, uint64_t astomach, uint64_t alegs, 
                        uint64_t dhead, uint64_t dgroin, uint64_t dchest, uint64_t dstomach, uint64_t dlegs)
{
    require_auth(MAINCONTRACT);

    conf config(MAINCONTRACT, MAINCONTRACT.value);
    _cstate = config.exists() ? config.get() : chanceset{};
    _cstate.ahead = ahead;
    _cstate.agroin = agroin;
    _cstate.achest = achest;
    _cstate.astomach = astomach;
    _cstate.alegs = alegs;
    _cstate.dhead = dhead;
    _cstate.dgroin = dgroin;
    _cstate.dchest = dchest;
    _cstate.dstomach = dstomach;
    _cstate.dlegs = dlegs;
    config.set(_cstate, MAINCONTRACT);
}

ACTION darkcountryf::resetstats()
{
    require_auth(MAINCONTRACT);
    killtables _kills(MAINCONTRACT, MAINCONTRACT.value);
    auto kill_itr = _kills.begin();
    while(kill_itr!=_kills.end())
    {
        kill_itr = _kills.erase(kill_itr);
    }
}


////////////////////////////////////////////////////////////
//              ROOM ACTIONS
////////////////////////////////////////////////////////////


//atomic transfer action -> create room/add to exists room
ACTION darkcountryf::transferatom(eosio::name from, eosio::name to, std::vector<uint64_t> asset_ids, std::string memo)
{
    require_auth(from);

    if (to != MAINCONTRACT)
    return;

    eosio::check(!asset_ids.empty(), "Vector is empty");
    eosio::check(memo.size() <= 256, "memo has more than 256 bytes");
    eosio::check(asset_ids.size() == 3, "Sorry, you must transfer three heroes on one transaction.");

    assets _atomicass(eosio::name("atomicassets"), MAINCONTRACT.value);
    auto asset_itr = _atomicass.find(asset_ids.at(0));
    eosio::check(asset_itr != _atomicass.end(), "Can't find asset.");
    eosio::check(asset_itr->collection_name == HEROCONTRACT, "Wrong asset collection.");
    eosio::check((asset_itr->schema_name == eosio::name("rareheroes")) || (asset_itr->schema_name == eosio::name("epicheroes")) || (asset_itr->schema_name == eosio::name("legheroes")) || (asset_itr->schema_name == eosio::name("mythheroes")) || (asset_itr->schema_name == eosio::name("dcheroes")),"Wrong asset(schema).");

    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(from.value);
    eosio::check(user_itr == _usersingames.end(),"User plays game now.");

    //check are all items with one rarity
    for(int i = 1; i < asset_ids.size(); i++)
    {
        asset_itr = _atomicass.find(asset_ids.at(i));
        eosio::check(asset_itr != _atomicass.end(), "Can't find asset.");
        eosio::check(asset_itr->collection_name == HEROCONTRACT, "Wrong asset collection.");

        asset_itr = _atomicass.find(asset_ids.at(1));
        eosio::check((asset_itr->schema_name == eosio::name("rareheroes")) || (asset_itr->schema_name == eosio::name("epicheroes")) || (asset_itr->schema_name == eosio::name("legheroes")) || (asset_itr->schema_name == eosio::name("mythheroes")) || (asset_itr->schema_name == eosio::name("dcheroes")),"Wrong asset(schema).");
    }

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.begin();

    if(room_itr == _rooms.end())
    {
        eosio::action createRoom = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            MAINCONTRACT,
            eosio::name("createroom"),
            std::make_tuple(from, asset_ids, (uint64_t)eosio::current_time_point().sec_since_epoch())
        );
        createRoom.send();
    }
    else
    {
        while(room_itr != _rooms.end())
        {
            if (room_itr->username2 == eosio::name("wait") && room_itr->gametype == 0)
            {
                eosio::action addToRoom = eosio::action(
                    eosio::permission_level{MAINCONTRACT, eosio::name("active")},
                    MAINCONTRACT,
                    eosio::name("addtoroom"),
                    std::make_tuple(room_itr->roomid, from, asset_ids, (uint64_t)eosio::current_time_point().sec_since_epoch(),0));
                addToRoom.send();
                break;
            }
            else
            {
                room_itr++;
            }

            if(room_itr == _rooms.end())
            {
                eosio::action createRoom = eosio::action(
                    eosio::permission_level{MAINCONTRACT, eosio::name("active")},
                    MAINCONTRACT,
                    eosio::name("createroom"),
                    std::make_tuple(from, asset_ids, (uint64_t)eosio::current_time_point().sec_since_epoch(),0));
                createRoom.send();
                break;
            }
        }
    }

}


//atomic transfer action -> create room/add to exists room
ACTION darkcountryf::addheroes(eosio::name from, std::vector<uint64_t> asset_ids)
{
    require_auth(from);

    eosio::check(asset_ids.size() == 3, "Sorry, you must transfer three heroes on one transaction.");

    assets _atomicass(eosio::name("atomicassets"), from.value);
    
    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(from.value);
    eosio::check(user_itr == _usersingames.end(),"User plays game now.");

    heroes _heroes(MAINCONTRACT, MAINCONTRACT.value);

    //check are all items with one rarity
    for(int i = 0; i < asset_ids.size(); i++)
    {
        auto asset_itr = _atomicass.find(asset_ids.at(i));
        eosio::check(asset_itr != _atomicass.end(), "Can't find asset.");
        eosio::check(asset_itr->collection_name == HEROCONTRACT, "Wrong asset collection.");
        eosio::check((asset_itr->schema_name == eosio::name("rareheroes")) || (asset_itr->schema_name == eosio::name("epicheroes")) || (asset_itr->schema_name == eosio::name("legheroes")) || (asset_itr->schema_name == eosio::name("mythheroes")) || (asset_itr->schema_name == eosio::name("dcheroes")),"Wrong asset(schema).");

        auto hero_itr = _heroes.find(asset_ids.at(i));
        eosio::check(hero_itr == _heroes.end(),"Hero is in game.");
        _heroes.emplace(MAINCONTRACT,[&](auto& new_hero){
            new_hero.heroid = asset_ids.at(i);
        });
    }

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.begin();

    if(room_itr == _rooms.end())
    {
        eosio::action createRoom = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            MAINCONTRACT,
            eosio::name("createroom"),
            std::make_tuple(from, asset_ids, (uint64_t)eosio::current_time_point().sec_since_epoch(),1)
        );
        createRoom.send();
    }
    else
    {
        while(room_itr != _rooms.end())
        {
            if (room_itr->username2 == eosio::name("wait") && room_itr->gametype == 1)
            {
                eosio::action addToRoom = eosio::action(
                    eosio::permission_level{MAINCONTRACT, eosio::name("active")},
                    MAINCONTRACT,
                    eosio::name("addtoroom"),
                    std::make_tuple(room_itr->roomid, from, asset_ids, (uint64_t)eosio::current_time_point().sec_since_epoch(),1));
                addToRoom.send();
                break;
            }
            else
            {
                room_itr++;
            }

            if(room_itr == _rooms.end())
            {
                eosio::action createRoom = eosio::action(
                    eosio::permission_level{MAINCONTRACT, eosio::name("active")},
                    MAINCONTRACT,
                    eosio::name("createroom"),
                    std::make_tuple(from, asset_ids, (uint64_t)eosio::current_time_point().sec_since_epoch(),1));
                createRoom.send();
                break;
            }
        }
    }

}





ACTION darkcountryf::returnheroes(eosio::name username, uint64_t roomid)
{
    require_auth(username);

    rooms _rooms(MAINCONTRACT,MAINCONTRACT.value);
    auto room_itr = _rooms.find(roomid);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");
    eosio::check((room_itr->status == 100)||(room_itr->status == 101),"Game was started.");

    if(room_itr->timestamp+90 <= (uint64_t)eosio::current_time_point().sec_since_epoch() && room_itr->username2==eosio::name("wait"))
    {
        if(room_itr->gametype == 0)
        {
            eosio::action returnHeroes = eosio::action(
                eosio::permission_level{MAINCONTRACT, eosio::name("active")},
                ATOMICCONTRACT,
                eosio::name("transfer"),
                std::make_tuple(MAINCONTRACT, room_itr->username1, room_itr->nftheroes1,std::string("Return heroes from Heroes Fight.")));
            returnHeroes.send();
        }
        else if (room_itr->gametype == 1)
        {
            heroes _heroes(MAINCONTRACT, MAINCONTRACT.value);
            for (int i = 0; i < room_itr->nftheroes1.size(); i++)
            {
                auto hero_itr = _heroes.find(room_itr->nftheroes1.at(i));
                eosio::check(hero_itr != _heroes.end(),"Hero doesn't exist in game.");
                _heroes.erase(hero_itr);
            }
        }

        eosio::action deleteRoom = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            MAINCONTRACT,
            eosio::name("deleteroom"),
            std::make_tuple(roomid));
        deleteRoom.send();


    }
    else
    {
        eosio::check(0,"Sorry, you must wain 3 min, before take your heroes back.");
    }
}


ACTION darkcountryf::deserialize(eosio::name username, hero_s hero1, hero_s hero2, hero_s hero3)
{
    require_auth(MAINCONTRACT);

    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr != _usersingames.end(),"User plays game now.");

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(user_itr->roomid);
    eosio::check(room_itr != _rooms.end(),"Room wasn't created.");

    std::vector<hero_s> heroes;
    heroes.push_back(hero1);
    heroes.push_back(hero2);
    heroes.push_back(hero3);

    if((room_itr->status == 101)||(room_itr->username1 == username))
    {
        _rooms.modify(room_itr, MAINCONTRACT, [&](auto& new_room){
            new_room.heroes1 = heroes;
            new_room.timestamp = eosio::current_time_point().sec_since_epoch();
            if(new_room.status == 101)
            new_room.status = 100;
        });
    }
    else if((room_itr->status == 102)&&(room_itr->username2 == username))
    {
        _rooms.modify(room_itr, MAINCONTRACT, [&](auto& new_room){
            new_room.heroes2 = heroes;
            new_room.timestamp = eosio::current_time_point().sec_since_epoch();
            if((new_room.status == 102) && (!new_room.heroes1.empty()))
            new_room.status = 0;
        });
    }
}

////////////////////////////////////////////////////////////
//              ROOM ACTIONS
////////////////////////////////////////////////////////////

ACTION darkcountryf::createroom(eosio::name username, std::vector<uint64_t> heroes, uint64_t timestamp, uint8_t gametype)
{
    require_auth(MAINCONTRACT);


    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr == _usersingames.end(),"User plays game now.");

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    uint64_t room_id = getroomid();
    auto room_itr = _rooms.find(room_id);
    eosio::check(room_itr == _rooms.end(),"Room was created.");


    _rooms.emplace(MAINCONTRACT, [&](auto& new_room){
        new_room.roomid = room_id;
        new_room.username1 = username;
        new_room.nftheroes1 = heroes;
        new_room.timestamp = timestamp;
        new_room.username2 = eosio::name("wait");
        new_room.status = 101;
        new_room.gametype = gametype;
    });


    _usersingames.emplace(MAINCONTRACT, [&](auto& new_user){
        new_user.username = username;
        new_user.roomid = room_id;
    });
}

ACTION darkcountryf::addtoroom(uint64_t roomid, eosio::name username, std::vector<uint64_t> heroes, uint64_t timestamp, uint8_t gametype)
{
    require_auth(MAINCONTRACT);

    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr == _usersingames.end(),"User plays game now.");

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(roomid);
    eosio::check(room_itr != _rooms.end(),"Room wasn't created.");
    eosio::check(room_itr->gametype == gametype,"Wrong room game type.");

    _rooms.modify(room_itr, MAINCONTRACT, [&](auto& new_room){
        new_room.username2 = username;
        new_room.nftheroes2 = heroes;
        new_room.timestamp = timestamp;
        new_room.status = 102;
    });


    _usersingames.emplace(MAINCONTRACT, [&](auto& new_user){
        new_user.username = username;
        new_user.roomid = room_itr->roomid;
    });
}

ACTION darkcountryf::deleteroom(uint64_t roomid)
{
    require_auth(MAINCONTRACT);

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(roomid);
    eosio::check(room_itr != _rooms.end(),"Room wasn't created.");
    eosio::check((room_itr->status == 4)||(room_itr->username2 == eosio::name("wait")||(room_itr->status == 101)||(room_itr->status == 100)) ,"Game wasn't ended.");

    usersingames _usersingames(MAINCONTRACT,MAINCONTRACT.value);
    auto user_itr = _usersingames.find(room_itr->username1.value);
    _usersingames.erase(user_itr);
    
    _rooms.erase(room_itr);

    fights _fights(MAINCONTRACT,MAINCONTRACT.value);
    auto fight_itr = _fights.find(roomid);
    if(fight_itr != _fights.end())
    {
        _fights.erase(fight_itr);
    }
}



////////////////////////////////////////////////////////////
//              FIGHT ACTIONS
////////////////////////////////////////////////////////////

ACTION darkcountryf::turn(eosio::name username, std::string attack, std::string blocktype, bool specialatk)
{
    require_auth(username);

    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr != _usersingames.end(),"User doesn't play game now.");

    rooms _rooms(MAINCONTRACT,MAINCONTRACT.value);
    auto room_itr = _rooms.find(user_itr->roomid);
    eosio::check(room_itr != _rooms.end(),"Couldn't find room.");
    eosio::check((room_itr->username1 == username)||(room_itr->username2 == username),"Current user doesn't play in this room.");
    eosio::check((room_itr->status == 0)||(room_itr->status == 1),"Fight phase was ended.");

    if(room_itr->username1 == username)
    {    
        uint8_t pos = findhero(room_itr->heroes1);
        if(specialatk)
        eosio::check(room_itr->heroes1.at(pos).energy >=25,"Your hero haven't enought energy for special attack");
    }
    else
    {
        uint8_t pos = findhero(room_itr->heroes2);
        if(specialatk)
        eosio::check(room_itr->heroes2.at(pos).energy >=25,"Your hero haven't enought energy for special attack");
    }

    fights _fights(MAINCONTRACT, MAINCONTRACT.value);
    auto fight_itr = _fights.find(user_itr->roomid);



    if(fight_itr == _fights.end())
    {
        _fights.emplace(MAINCONTRACT, [&] (auto& new_fight){
            new_fight.roomid = user_itr->roomid;
            if(room_itr->username1 == username)
            {
                new_fight.firstuser.username = username;
                new_fight.firstuser.heroname = getheroname(room_itr->heroes1); 
                new_fight.firstuser.attacktype = attack;
                new_fight.firstuser.blocktype = blocktype;
                new_fight.firstuser.energy = (specialatk)?25:0;
            }
            else if(room_itr->username2 == username)
            {
                new_fight.seconduser.username = username;
                new_fight.seconduser.heroname = getheroname(room_itr->heroes2); 
                new_fight.seconduser.attacktype = attack;
                new_fight.seconduser.blocktype = blocktype;
                new_fight.seconduser.energy = (specialatk)?25:0;
            }
            else
            {
                eosio::check(0,"Something come wrong...");
            }
        });
    }
    else
    {
        _fights.modify(fight_itr, MAINCONTRACT,[&](auto& mod_fight){
            if(room_itr->username1 == username)
            {
                mod_fight.firstuser.username = username;
                mod_fight.firstuser.heroname = getheroname(room_itr->heroes1); 
                mod_fight.firstuser.attacktype = attack;
                mod_fight.firstuser.blocktype = blocktype;
                mod_fight.firstuser.energy = (specialatk)?25:0;


                if(room_itr->status == 0)
                {
                    mod_fight.seconduser.heroname.clear(); 
                    mod_fight.seconduser.attacktype = "0";
                    mod_fight.seconduser.blocktype = "0";
                    mod_fight.seconduser.energy = 0;
                }
            }
            else if(room_itr->username2 == username)
            {
     
                mod_fight.seconduser.username = username;
                mod_fight.seconduser.heroname = getheroname(room_itr->heroes2); 
                mod_fight.seconduser.attacktype = attack;
                mod_fight.seconduser.blocktype = blocktype;
                mod_fight.seconduser.energy = (specialatk)?25:0;


                if(room_itr->status == 0)
                {
                    mod_fight.firstuser.heroname.clear(); 
                    mod_fight.firstuser.attacktype = "0";
                    mod_fight.firstuser.blocktype = "0";
                    mod_fight.firstuser.energy = 0;
                }
            }
            else
            {
                eosio::check(0,"Something comes wrong...");
            }
        });
    }


    _rooms.modify(room_itr, MAINCONTRACT, [&](auto& mod_room){
        if (mod_room.status == 0)
        {
            mod_room.status = 1;
            mod_room.round++;
        }
        else
        {
            mod_room.status = 2;
        }
        mod_room.timestamp = eosio::current_time_point().sec_since_epoch();
    });

}

ACTION darkcountryf::endturn(eosio::name username)
{
    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr != _usersingames.end(),"User doesn't play game now.");

    rooms _rooms(MAINCONTRACT,MAINCONTRACT.value);
    auto room_itr = _rooms.find(user_itr->roomid);
    eosio::check(room_itr != _rooms.end(),"Couldn't find room.");
    eosio::check((room_itr->username1 == username)||(room_itr->username2 == username),"Current user doesn't play in this room.");
    eosio::check(room_itr->status == 1,"In this game phase you can't end turn.");
    eosio::check(room_itr->timestamp+30 <= (uint64_t)eosio::current_time_point().sec_since_epoch(),"Sorry, but you can end turn only, when opponent have not done his turn for 30 sec.");

    fights _fights(MAINCONTRACT, MAINCONTRACT.value);
    auto fight_itr = _fights.find(user_itr->roomid);
    eosio::check(fight_itr != _fights.end(),"Fight doesn't finded.");

    //TODO not empty
    if(!fight_itr->firstuser.heroname.empty())
    {
        require_auth(fight_itr->firstuser.username);
        _fights.modify(fight_itr, MAINCONTRACT,[&](auto& mod_fight){
            mod_fight.seconduser.username = room_itr->username2;
            mod_fight.seconduser.heroname = getheroname(room_itr->heroes2);
            mod_fight.seconduser.energy = 0;
            mod_fight.seconduser.damage = 0;
            mod_fight.seconduser.attacktype = "0";
            mod_fight.seconduser.blocktype = "0";   
        });
    }
    else if(!fight_itr->seconduser.heroname.empty())
    {
        require_auth(fight_itr->seconduser.username);
        _fights.modify(fight_itr, MAINCONTRACT,[&](auto& mod_fight){
            mod_fight.firstuser.username = room_itr->username1;
            mod_fight.firstuser.heroname = getheroname(room_itr->heroes1);
            mod_fight.firstuser.energy = 0;
            mod_fight.firstuser.damage = 0;
            mod_fight.firstuser.attacktype = "0";
            mod_fight.firstuser.blocktype = "0";   
        });
    }
    else
    {
        return;
    }

    _rooms.modify(room_itr, MAINCONTRACT, [&](auto& mod_room){
        mod_room.status = 2;
        mod_room.timestamp = eosio::current_time_point().sec_since_epoch();
    });
}

ACTION darkcountryf::fight(uint64_t roomid, eosio::name username1, std::string attacktype1, std::string block1, 
                eosio::name username2, std::string attacktype2, std::string block2)
{
    require_auth(MAINCONTRACT);

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(roomid);
    eosio::check(room_itr != _rooms.end(),"Couldn't find room.");
    eosio::check((room_itr->username1 == username1)&&(room_itr->username2 == username2),"One of users doesn't play in this room.");
    eosio::check(room_itr->status == 2,"Fight phase wasn't ended.");

    fights _fights(MAINCONTRACT, MAINCONTRACT.value);
    auto fight_itr = _fights.find(roomid);
    eosio::check(fight_itr != _fights.end(),"No active fights on this room.");
    eosio::check((fight_itr->firstuser.username == username1)&&(fight_itr->seconduser.username == username2),"Some username is wrong.");
    
    _fights.modify(fight_itr, MAINCONTRACT, [&](auto& mod_fight){
        mod_fight.firstuser.attacktype = attacktype1;
        mod_fight.firstuser.blocktype = block1;
        mod_fight.seconduser.attacktype = attacktype2;
        mod_fight.seconduser.blocktype = block2;
    });

    //create singing value
    auto size = eosio::transaction_size();
    char buf[size];

    auto read = eosio::read_transaction(buf, size);
    eosio::check(size == read, "read_transaction() has failed.");

    auto tx_signing_value = eosio::sha256(buf, size);

    auto byte_array = tx_signing_value.extract_as_byte_array();
    uint64_t signing_value = 0;
    for (int k = 0; k < 8; k++)
    {
        signing_value <<= 8;
        signing_value |= (uint64_t)byte_array[k];
    }


    eosio::action getRandom = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            eosio::name("orng.wax"),
            eosio::name("requestrand"),
            std::make_tuple(roomid, signing_value, MAINCONTRACT));
        getRandom.send();
}


ACTION darkcountryf::receiverand (uint64_t customer_id, const eosio::checksum256 &random_value)
{
    require_auth(eosio::name("orng.wax"));

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(customer_id);
    eosio::check(room_itr != _rooms.end(),"Couldn't find room.");
    eosio::check(room_itr->status == 2,"Fight phase wasn't ended.");

    fights _fights(MAINCONTRACT, MAINCONTRACT.value);
    auto fight_itr = _fights.find(customer_id);
    eosio::check(fight_itr != _fights.end(),"No active fights on this room.");

    //get random values
    uint64_t max_value = 1000;
    auto byte_array = random_value.extract_as_byte_array();

    uint64_t random_int = 0;
    for (int i = 0; i < 8; i++) {
        random_int <<= 8;
        random_int |= (uint64_t)byte_array[i];
    }

    uint64_t num1 = random_int % max_value;

    random_int = 0;
    for (int i = 8; i < 16; i++) {
        random_int <<= 8;
        random_int |= (uint64_t)byte_array[i];
    }

    uint64_t num2 = random_int % max_value;

    uint8_t pos1 = findhero(room_itr->heroes1);
    uint8_t pos2 = findhero(room_itr->heroes2);

    uint64_t damage1 = setdamage(num1, fight_itr->firstuser.attacktype, fight_itr->seconduser.blocktype, (bool)fight_itr->firstuser.energy, room_itr->heroes1.at(pos1).rarity);
    uint64_t damage2 = setdamage(num2, fight_itr->seconduser.attacktype, fight_itr->firstuser.blocktype, (bool)fight_itr->seconduser.energy, room_itr->heroes2.at(pos2).rarity);



    _rooms.modify(room_itr, MAINCONTRACT, [&](auto& mod_room){
        if(mod_room.heroes1.at(pos1).health > damage2)
        {
            mod_room.heroes1.at(pos1).health -= damage2;
        }
        else
        {
            mod_room.heroes1.at(pos1).health = 0;
            if(pos1 == 2)
            {
                mod_room.status = 4;
            }
        }
        if(mod_room.heroes2.at(pos2).health > damage1)
        {
            mod_room.heroes2.at(pos2).health -= damage1;
        }
        else
        {
            mod_room.heroes2.at(pos2).health = 0;
            if(pos2 == 2)
            {
                mod_room.status = 4;
            }
        }
        if(fight_itr->firstuser.energy != 0)
        mod_room.heroes1.at(pos1).energy -= fight_itr->firstuser.energy;
        if(fight_itr->seconduser.energy != 0)
        mod_room.heroes2.at(pos2).energy -= fight_itr->seconduser.energy;
        if(mod_room.status == 2)
        {
            mod_room.status = 0;
        }
        mod_room.timestamp = eosio::current_time_point().sec_since_epoch();
    });

    killtables _kills(MAINCONTRACT, MAINCONTRACT.value);
    auto kill_itr1 = _kills.find(room_itr->username1.value);
    if(kill_itr1 == _kills.end())
    {
        _kills.emplace(MAINCONTRACT,[&](auto& new_kill){
            new_kill.username = room_itr->username1;
            new_kill.totaldamage = damage1;
            if(room_itr->heroes2.at(pos2).health == 0)
            new_kill.kills = 1;
        });
    }
    else 
    {
        _kills.modify(kill_itr1, MAINCONTRACT,[&](auto& new_kill){
            new_kill.totaldamage += damage1;
            if(room_itr->heroes2.at(pos2).health == 0)
            new_kill.kills++;
        });
    }



    auto kill_itr2 = _kills.find(room_itr->username2.value);
    if(kill_itr2 == _kills.end())
    {
        _kills.emplace(MAINCONTRACT,[&](auto& new_kill){
            new_kill.username = room_itr->username2;
            new_kill.totaldamage = damage2;
            if(room_itr->heroes1.at(pos1).health == 0)
            new_kill.kills = 1;
        });
    }
    else 
    {
        _kills.modify(kill_itr2, MAINCONTRACT,[&](auto& new_kill){
            new_kill.totaldamage += damage2;
            if(room_itr->heroes1.at(pos1).health == 0)
            new_kill.kills++;
        });
    }

    _fights.modify(fight_itr,MAINCONTRACT,[&](auto& mod_fight){
        mod_fight.firstuser.damage = damage1;
        mod_fight.seconduser.damage = damage2;
    });

    eosio::action logFight = eosio::action(
        eosio::permission_level{MAINCONTRACT, eosio::name("active")},
        MAINCONTRACT,
        eosio::name("logfight"),
        std::make_tuple(room_itr->username1, room_itr->heroes1.at(pos1).heroname, damage1, num1, room_itr->username2, room_itr->heroes2.at(pos2).heroname, damage2, num2));
    logFight.send();

    if(room_itr->status == 4)
    {
        eosio::action logFight = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            MAINCONTRACT,
            eosio::name("endgame"),
            std::make_tuple(room_itr->roomid));
        logFight.send();
    }
}


ACTION darkcountryf::endgame(uint64_t roomid)
{
    require_auth(MAINCONTRACT);

    rooms _rooms(MAINCONTRACT,MAINCONTRACT.value);
    auto room_itr = _rooms.find(roomid);
    eosio::check(room_itr->status == 4,"Game wasn't ended.");

    usersingames _usersingames(MAINCONTRACT,MAINCONTRACT.value);
    auto user_itr = _usersingames.find(room_itr->username1.value);
    _usersingames.erase(user_itr);
    user_itr = _usersingames.find(room_itr->username2.value);
    _usersingames.erase(user_itr);

    eosio::name winner, loser;
    int win_hero_points = 0, lose_hero_points = 0;

    if(room_itr->heroes1[2].health == 0)
    {
        winner = room_itr->username2;
        win_hero_points = setheropoints(room_itr->heroes2);
        loser = room_itr->username1;
        lose_hero_points = setheropoints(room_itr->heroes1);
    }
    else
    {
        loser = room_itr->username2;
        lose_hero_points = setheropoints(room_itr->heroes2);
        winner = room_itr->username1;
        win_hero_points = setheropoints(room_itr->heroes1);
    }


    lboards _lboards(NESTCONTRACT, NESTCONTRACT.value);
    auto lb_itr = _lboards.find(2);
    eosio::check(lb_itr != _lboards.end(),"Leader board doesn't exist.");

    auto pos1 = finder(lb_itr->players,winner);
    double win_points;
    if (pos1 != -1)
    {
        win_points = lb_itr->players.at(pos1).points;
    }
    else
    {
        win_points = 0;
    }

    auto pos2 = finder(lb_itr->players,loser);
    double lose_points;
    if (pos2 != -1)
    {
        lose_points = lb_itr->players.at(pos2).points;
    }
    else
    {
        lose_points = 0;
    }


    int end_hero_points = win_hero_points - lose_hero_points;

    if(win_points > lose_points)
    {
        eosio::action updatePoint = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, winner, (double)(23-end_hero_points),std::string("")));
        updatePoint.send();

        double del_points;
        if(lose_points >= 22.0)
        {
            del_points = 22.0;
        }
        else
        {
            del_points = lose_points;
        }
        del_points *= -1;

        eosio::action update1Point = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, loser, (double)(del_points+end_hero_points),std::string("")));
        update1Point.send();
    }
    else if (win_points < lose_points)
    {
        eosio::action updatePoint = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, winner, (double)(27-end_hero_points),std::string("")));
        updatePoint.send();

        double del_points;
        if(lose_points >= 24.0)
        {
            del_points = 24.0;
        }
        else
        {
            del_points = lose_points;
        }
        del_points *= -1;
        
        eosio::action update1Point = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, loser, (double)(del_points+end_hero_points),std::string("")));
        update1Point.send();
    }
    else
    {
        eosio::action updatePoint = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, winner, (double)(23-end_hero_points),std::string("")));
        updatePoint.send();


        double del_points;
        if(lose_points >= 22.0)
        {
            del_points = 22.0;
        }
        else
        {
            del_points = lose_points;
        }
        del_points *= -1;
        
        eosio::action update1Point = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, loser, (double)(del_points+end_hero_points),std::string("")));
        update1Point.send();

    }

    if(room_itr->gametype == 0)
    {
        eosio::action returnHeroes = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            ATOMICCONTRACT,
            eosio::name("transfer"),
            std::make_tuple(MAINCONTRACT, room_itr->username1, room_itr->nftheroes1,std::string("Return heroes from Heroes Fight.")));
        returnHeroes.send();

        eosio::action return1Heroes = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            ATOMICCONTRACT,
            eosio::name("transfer"),
            std::make_tuple(MAINCONTRACT, room_itr->username2, room_itr->nftheroes2,std::string("Return heroes from Heroes Fight.")));
        return1Heroes.send();
    } 
    else if(room_itr->gametype == 1)
    {
        heroes _heroes(MAINCONTRACT, MAINCONTRACT.value);
        for (int i = 0; i < room_itr->nftheroes1.size(); i++)
        {
            auto hero_itr = _heroes.find(room_itr->nftheroes1.at(i));
            eosio::check(hero_itr != _heroes.end(),"Hero doesn't exist in game.");
            _heroes.erase(hero_itr);
        }
        for (int i = 0; i < room_itr->nftheroes2.size(); i++)
        {
            auto hero_itr = _heroes.find(room_itr->nftheroes2.at(i));
            eosio::check(hero_itr != _heroes.end(),"Hero doesn't exist in game.");
            _heroes.erase(hero_itr);
        }
    }

    eosio::action gameLog = eosio::action(
        eosio::permission_level{MAINCONTRACT, eosio::name("active")},
        MAINCONTRACT,
        eosio::name("endgamelog"),
        std::make_tuple(room_itr->roomid, winner, loser));
    gameLog.send();


}


ACTION darkcountryf::cleanrooms()
{
    require_auth(MAINCONTRACT);

    rooms _rooms(MAINCONTRACT,MAINCONTRACT.value);
    auto room_itr = _rooms.begin();

    fights _fights(MAINCONTRACT,MAINCONTRACT.value);

    while(room_itr != _rooms.end())
    {
        if((room_itr->status == 4) && (room_itr->timestamp+150 <= (uint64_t)eosio::current_time_point().sec_since_epoch()))
        {
            auto fight_itr = _fights.find(room_itr->roomid);
            if(fight_itr != _fights.end())
            {
                _fights.erase(fight_itr);
            }
            room_itr = _rooms.erase(room_itr);
        }
        else if ((room_itr->status != 4)&&(room_itr->timestamp+300 <= (uint64_t)eosio::current_time_point().sec_since_epoch()))
        {
             eosio::action returnHeroes = eosio::action(
                eosio::permission_level{MAINCONTRACT, eosio::name("active")},
                MAINCONTRACT,
                eosio::name("delgame"),
                std::make_tuple(room_itr->roomid));
            returnHeroes.send();
        }
            room_itr++;
        
    }
}

////////////////////////////////////////////////////////////
//              HELP FUNCTIONS
////////////////////////////////////////////////////////////

ACTION darkcountryf::logfight(eosio::name username1, std::string heroname1, uint64_t damage1, uint64_t randnumber1,
                    eosio::name username2, std::string heroname2, uint64_t damage2, uint64_t randnumber2)
{
    require_auth(MAINCONTRACT);

    require_recipient(username1);
    require_recipient(username2);
}


ACTION darkcountryf::delgame(uint64_t id)
{
    require_auth(MAINCONTRACT);

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(id);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");

     if(room_itr->gametype == 0)
    {
        eosio::action returnHeroes = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            ATOMICCONTRACT,
            eosio::name("transfer"),
            std::make_tuple(MAINCONTRACT, room_itr->username1, room_itr->nftheroes1,std::string("Return heroes from Heroes Fight.")));
        returnHeroes.send();

        if(room_itr->username2!=eosio::name("wait"))
        {
            eosio::action return1Heroes = eosio::action(
                eosio::permission_level{MAINCONTRACT, eosio::name("active")},
                ATOMICCONTRACT,
                eosio::name("transfer"),
                std::make_tuple(MAINCONTRACT, room_itr->username2, room_itr->nftheroes2,std::string("Return heroes from Heroes Fight.")));
            return1Heroes.send();
    
        }
    } 
    else if(room_itr->gametype == 1)
    {
        heroes _heroes(MAINCONTRACT, MAINCONTRACT.value);
        for (int i = 0; i < room_itr->nftheroes1.size(); i++)
        {
            auto hero_itr = _heroes.find(room_itr->nftheroes1.at(i));
            eosio::check(hero_itr != _heroes.end(),"Hero doesn't exist in game.");
            _heroes.erase(hero_itr);
        }
        for (int i = 0; i < room_itr->nftheroes2.size(); i++)
        {
            auto hero_itr = _heroes.find(room_itr->nftheroes2.at(i));
            eosio::check(hero_itr != _heroes.end(),"Hero doesn't exist in game.");
            _heroes.erase(hero_itr);
        }
    }
    
    _rooms.erase(room_itr);

    fights _fights(MAINCONTRACT, MAINCONTRACT.value);
    auto fitr = _fights.find(id);
    if(fitr != _fights.end())
    {
        _fights.erase(fitr);
    }
    usersingames _usersingames(MAINCONTRACT,MAINCONTRACT.value);
    auto user_itr = _usersingames.find(room_itr->username1.value);
    _usersingames.erase(user_itr);

    if(room_itr->username2!=eosio::name("wait"))
    {
        user_itr = _usersingames.find(room_itr->username2.value);
        _usersingames.erase(user_itr);
    }
}

ACTION darkcountryf::deluser(eosio::name username)
{
    require_auth(MAINCONTRACT);

    usersingames _usersingames(MAINCONTRACT,MAINCONTRACT.value);
    auto itr = _usersingames.find(username.value);

    _usersingames.erase(itr);
}


ACTION darkcountryf::endgamelog(uint64_t roomid, eosio::name winner, eosio::name loser)
{
    require_auth(MAINCONTRACT);

    require_recipient(winner);
    require_recipient(loser);
}

//help functions
uint64_t darkcountryf::getroomid()
{
    gconf config(MAINCONTRACT, MAINCONTRACT.value);
    _gstate = config.exists() ? config.get() : gameset{};
    _gstate.roomid++;
    config.set(_gstate, MAINCONTRACT);

    return _gstate.roomid;
}


std::string darkcountryf::getheroname(std::vector<hero_s> heroes)
{
    for (int i = 0;i < heroes.size(); i++)
    {
        if(heroes.at(i).health != 0)
        {
            return heroes.at(i).heroname;
        }
    }
    return "0";
}


uint64_t darkcountryf::setdamage(uint64_t randnumb, std::string attacktype, std::string blocktype, bool specialatk, uint8_t rarity)
{
    conf config(MAINCONTRACT, MAINCONTRACT.value);
    eosio::check(config.exists(),"Chances must be settuped");
    _cstate = config.get();

    double base = 0.0;
    if (rarity == 1)
    {
        base = 3.0;
    }
    else if (rarity == 2)
    {
        base = 3.3;
    }
    else if (rarity == 3)
    {
        base = 3.9;
    }
    else if (rarity == 4)
    {
        base = 4.5;
    }
    else if (rarity == 5)
    {
        base = 6.0;
    }

    if(attacktype == "1")
    {
        (specialatk) ? base *= 2 : base *= 1;
        if((blocktype == "1")||(blocktype == "2"))
        {
            base /= 2;
        }
        return (uint64_t)base;
    }
    else if(attacktype == "2")
    {   
        if(randnumb<_cstate.agroin)
        {
            base *= 4;
            (specialatk) ? base *= 2 : base *= 1;
            if((blocktype == "2")||(blocktype == "3"))
            {
                base /= 2;
            }
            return (uint64_t)base;
        }
    }
    else if(attacktype == "3")
    {   
        if(randnumb<_cstate.astomach)
        {
            base *= 2;
            (specialatk) ? base *= 2 : base *= 1;
            if((blocktype == "4")||(blocktype == "3"))
            {
                base /= 2;
            }
            return (uint64_t)base;
        }
    }
    else if(attacktype == "4")
    {   
        if(randnumb<_cstate.achest)
        {
            base *= 3;
            (specialatk) ? base *= 2 : base *= 1;
            if((blocktype == "4")||(blocktype == "5"))
            {
                base /= 2;
            }
            return (uint64_t)base;
        }
    }
    else if(attacktype == "5")
    {   
        if(randnumb<_cstate.ahead)
        {
            base *= 5;
            (specialatk) ? base *= 2 : base *= 1;
            if((blocktype == "1")||(blocktype == "5"))
            {
                base /= 2;
            }
            return (uint64_t)base;
        }
    }

    
    return 0;
}

uint8_t darkcountryf::findhero(std::vector<hero_s> herovec)
{
    for(int i = 0; i < herovec.size(); i++)
    {
        if(herovec.at(i).health > 0)
        {
            return i;
        }
    }
    return 2;
}


int darkcountryf::finder(std::vector<player_s> players, eosio::name username)
{
    for (uint64_t i = 0; i < players.size(); i++)
    {
        if (players.at(i).account == username)
        {
            return i;
        }
    }
    return -1;
}

int darkcountryf::setheropoints(std::vector<hero_s> heroes_ids)
{
    int points = 0;
    for(int i = 0; i < heroes_ids.size(); i++)
    {
        points += heroes_ids.at(i).rarity;
    }
    return points;
}

extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
  if (code == receiver)
  {
    switch (action)
    {
      EOSIO_DISPATCH_HELPER(darkcountryf, (resetstats)(createroom)(addtoroom)(deleteroom)(addheroes)(setchance)(returnheroes)(turn)(endturn)(fight)(receiverand)(delgame)(endgame)(deluser)(logfight)(cleanrooms)(deserialize)(endgamelog))
    }
  }
  else if (code == ATOMICCONTRACT.value && action == eosio::name("transfer").value)
  {
    execute_action(eosio::name(receiver), eosio::name(code), &darkcountryf::transferatom);
  }
  else
  {
    printf("Couldn't find an action.");
    return;
  }
}

