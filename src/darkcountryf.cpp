#include <darkcountryf.hpp>

//set fight properties
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


//reset tournament stats
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
//              ENERGY ACTIONS
////////////////////////////////////////////////////////////

//buy energy potion action(eosio.token transfer)
//if u want to add new potion - add new string parse
ACTION darkcountryf::transfer(eosio::name from, eosio::name to, eosio::asset quantity, std::string memo)               
{
    require_auth(from);
    
    if (to != MAINCONTRACT)
        return;

    eosio::check(quantity.symbol.code() == eosio::symbol_code("WAX"), "Invalid token");
    eosio::check(quantity.is_valid(), "invalid quantity");
    eosio::check(quantity.amount > 0, "must transfer positive quantity");
    eosio::check(memo.size() <= 256, "memo has more than 256 bytes");

    eosio::check(memo.at(0) == 'e',"Wrong memo e.");
    memo.erase(0,2);
    uint64_t num = std::stoi(memo);
    //check price
    eosio::check(quantity.amount == num*2000000000,"Wrong quantity.");

    energies _energy(MAINCONTRACT, MAINCONTRACT.value);
    auto energy_itr = _energy.find(from.value);
    if(energy_itr == _energy.end())
    {
        _energy.emplace(MAINCONTRACT, [&](auto& new_energy){
            new_energy.username = from;
            new_energy.number = num*10;
        });
    }
    else
    {
        _energy.modify(energy_itr, MAINCONTRACT, [&](auto& new_energy){
            new_energy.number += num*10;
        });
    }
}

//use energy on fight
ACTION darkcountryf::useenergy(uint64_t roomid, eosio::name username)
{
    (has_auth(username)) ? require_auth(username) : require_auth(MAINCONTRACT);

    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr != _usersingames.end(),"User is not playing game now.");

    //check number of user's energy potions
    energies _energ(MAINCONTRACT, MAINCONTRACT.value);
    auto ene_itr = _energ.find(username.value);
    eosio::check(ene_itr != _energ.end(),"User hasn't energy potions.");
    
    eosio::check(ene_itr->number > 0,"User han't energy potions.");
    

    _energ.modify(ene_itr, MAINCONTRACT, [&](auto& mod){
        mod.number--;
    });

    //add energy to hero
    rooms _rooms(MAINCONTRACT,MAINCONTRACT.value);
    auto room_itr = _rooms.find(user_itr->roomid);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");
    eosio::check((room_itr->username1 == username)||(room_itr->username2 == username),"Current user is not playing in this room.");
    
    uint8_t pos;

    if(room_itr->username1 == username)
    {    
        pos = findhero(room_itr->heroes1);
        eosio::check(room_itr->heroes1.at(pos).energy < 100,"potion set 100 energy.");
    }
    else
    {
        pos = findhero(room_itr->heroes2);
        eosio::check(room_itr->heroes2.at(pos).energy < 100,"potion set 100 energy.");
    }


    _rooms.modify(room_itr, MAINCONTRACT, [&](auto& mod_room){
        if(mod_room.username1 == username)
        {    
            mod_room.heroes1.at(pos).energy = 100;
        }
        else
        {
            mod_room.heroes2.at(pos).energy = 100;
        }
        mod_room.timestamp = eosio::current_time_point().sec_since_epoch();
    });



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

    eosio::check(!asset_ids.empty(), "No heroes transferred.");
    eosio::check(memo.size() <= 256, "memo has more than 256 bytes");
    eosio::check(asset_ids.size() == 3, "Sorry, you must transfer three heroes on one transaction.");

    assets _atomicass(eosio::name("atomicassets"), MAINCONTRACT.value);
    auto asset_itr = _atomicass.find(asset_ids.at(0));
    eosio::check(asset_itr != _atomicass.end(), "Asset cannot be found.");
    eosio::check(asset_itr->collection_name == HEROCONTRACT, "Wrong asset collection.");
    eosio::check((asset_itr->schema_name == eosio::name("rareheroes")) || (asset_itr->schema_name == eosio::name("epicheroes")) || (asset_itr->schema_name == eosio::name("legheroes")) || (asset_itr->schema_name == eosio::name("mythheroes")) || (asset_itr->schema_name == eosio::name("dcheroes")),"Wrong asset schema.");

    //check is user in game
    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(from.value);
    eosio::check(user_itr == _usersingames.end(),"User is in game already.");

    if(!memo.empty())
    {
        //add encryption
        uks _uks(MAINCONTRACT, MAINCONTRACT.value);

        auto k_itr = _uks.find(from.value);
        if(k_itr == _uks.end())
        {
            _uks.emplace(MAINCONTRACT, [&](auto& new_k){
                new_k.u = from;
                new_k.k = memo;
            });
        }
        else
        {
            _uks.modify(k_itr, MAINCONTRACT, [&](auto& mod_k)
            {
                mod_k.k = memo;
            });
        }
    }


    //check are all heroes with one rarity
    for(int i = 1; i < asset_ids.size(); i++)
    {
        asset_itr = _atomicass.find(asset_ids.at(i));
        eosio::check(asset_itr != _atomicass.end(), "Asset cannot be found.");
        eosio::check(asset_itr->collection_name == HEROCONTRACT, "Wrong asset collection.");

        asset_itr = _atomicass.find(asset_ids.at(1));
        eosio::check((asset_itr->schema_name == eosio::name("rareheroes")) || (asset_itr->schema_name == eosio::name("epicheroes")) || (asset_itr->schema_name == eosio::name("legheroes")) || (asset_itr->schema_name == eosio::name("mythheroes")) || (asset_itr->schema_name == eosio::name("dcheroes")),"Wrong asset schema.");
    }

    //find empty place in room or create new
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
ACTION darkcountryf::addheroes(eosio::name from, std::vector<uint64_t> asset_ids, std::string k)
{
    require_auth(from);

    eosio::check(asset_ids.size() == 3, "Sorry, you must pick three heroes.");

    assets _atomicass(eosio::name("atomicassets"), from.value);
    
    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(from.value);
    eosio::check(user_itr == _usersingames.end(),"User is in game already.");

    heroes _heroes(MAINCONTRACT, MAINCONTRACT.value);

    eosio::name collection;

    auto asset_itr = _atomicass.find(asset_ids.at(0));
    eosio::check(asset_itr != _atomicass.end(), "Can't find asset.");
    //check project heroes
    if (asset_itr->collection_name == HEROCONTRACT)
    {
        collection = HEROCONTRACT;
        eosio::check((asset_itr->schema_name == eosio::name("rareheroes")) || (asset_itr->schema_name == eosio::name("epicheroes")) || (asset_itr->schema_name == eosio::name("legheroes")) || (asset_itr->schema_name == eosio::name("mythheroes")) || (asset_itr->schema_name == eosio::name("dcheroes")),"Wrong asset(schema).");
    }
    else if (asset_itr->collection_name == BCHEROCONTRACT)
    {
        //eosio::check((from == eosio::name("woyqw.wam"))||(from == eosio::name("dctesttestdc"))||(from == eosio::name("pfkpw.wam"))||(from == eosio::name("h3kqu.wam")),"Wrong user.");
        collection = BCHEROCONTRACT;
        eosio::check(asset_itr->schema_name == eosio::name("series1"),"Wrong schema on BCH asset. Only series1.");
    }
    else if (asset_itr->collection_name == ALIENCONTRACT)
    {
        //eosio::check((from == eosio::name("woyqw.wam"))||(from == eosio::name("dctesttestdc")),"Wrong user.");
        collection = ALIENCONTRACT;
        eosio::check((asset_itr->schema_name == eosio::name("crew.worlds")) || (asset_itr->schema_name == eosio::name("faces.worlds")),"Wrong schema on Alien Worlds asset. Only faces.worlds and crew.worlds schemas.");
    }
    else if (asset_itr->collection_name == GPKCONTRACT)
    {
        //eosio::check((from == eosio::name("woyqw.wam"))||(from == eosio::name("dctesttestdc")),"Wrong user.");
        collection = GPKCONTRACT;
        eosio::check((asset_itr->schema_name == eosio::name("series1")) || (asset_itr->schema_name == eosio::name("series2")) || (asset_itr->schema_name == eosio::name("exotic")) || (asset_itr->schema_name == eosio::name("crashgordon")),"Wrong schema on GPK asset.");
    }
    else
    {
        eosio::check(0,"Asset has wrong schema");
    }
    
    uks _uks(MAINCONTRACT, MAINCONTRACT.value);

    auto k_itr = _uks.find(from.value);
    if(k_itr == _uks.end())
    {
        _uks.emplace(MAINCONTRACT, [&](auto& new_k){
            new_k.u = from;
            new_k.k = k;
        });
    }
    else
    {
        _uks.modify(k_itr, MAINCONTRACT, [&](auto& mod_k)
        {
            mod_k.k = k;
        });
    }

    //check are all items with one rarity
    for(int i = 0; i < asset_ids.size(); i++)
    {
        auto asset_itr = _atomicass.find(asset_ids.at(i));
        eosio::check(asset_itr != _atomicass.end(), "Asset cannot be found.");
        eosio::check(asset_itr->collection_name == collection, "Wrong asset collection.");
        if(collection == HEROCONTRACT)
        {
            eosio::check((asset_itr->schema_name == eosio::name("rareheroes")) || (asset_itr->schema_name == eosio::name("epicheroes")) || (asset_itr->schema_name == eosio::name("legheroes")) || (asset_itr->schema_name == eosio::name("mythheroes")) || (asset_itr->schema_name == eosio::name("dcheroes")),"Wrong asset schema.");
        }
        else if (collection == BCHEROCONTRACT)
        {
            eosio::check(asset_itr->schema_name == eosio::name("series1"),"Wrong schema on BCH asset. Only series1.");
        }
        else if (collection == ALIENCONTRACT)
        {
            eosio::check((asset_itr->schema_name == eosio::name("crew.worlds")) || (asset_itr->schema_name == eosio::name("faces.worlds")),"Wrong schema on Alien Worlds asset. Only faces.worlds and crew.worlds schemas.");
        }
        else if (collection == GPKCONTRACT)
        {
            eosio::check((asset_itr->schema_name == eosio::name("series1")) || (asset_itr->schema_name == eosio::name("series2")) || (asset_itr->schema_name == eosio::name("exotic")) || (asset_itr->schema_name == eosio::name("crashgordon")),"Wrong schema on GPK asset.");
        }

        auto hero_itr = _heroes.find(asset_ids.at(i));
        eosio::check(hero_itr == _heroes.end(),"Hero is in game already.");
        _heroes.emplace(MAINCONTRACT,[&](auto& new_hero){
            new_hero.heroid = asset_ids.at(i);
        });
    }

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.begin();
    //create room or enter in empty room
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

//if user don't find enemy , he can return heroes
ACTION darkcountryf::returnheroes(eosio::name username, uint64_t roomid)
{
    (has_auth(username)) ? require_auth(username) : require_auth(MAINCONTRACT);

    rooms _rooms(MAINCONTRACT,MAINCONTRACT.value);
    auto room_itr = _rooms.find(roomid);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");
    eosio::check((room_itr->status == 100)||(room_itr->status == 101),"Game was started already.");

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
        eosio::check(0,"You must wait 3 minutes to take your heroes back.");
    }
}

//deserialize heroes data, action called by server
ACTION darkcountryf::deserialize(eosio::name username, hero_s hero1, hero_s hero2, hero_s hero3)
{
    require_auth(MAINCONTRACT);

    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr != _usersingames.end(),"User is in game already.");

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(user_itr->roomid);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");

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

//create game room
ACTION darkcountryf::createroom(eosio::name username, std::vector<uint64_t> heroes, uint64_t timestamp, uint8_t gametype)
{
    require_auth(MAINCONTRACT);


    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr == _usersingames.end(),"User is in game already.");

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

//enter in empty room
ACTION darkcountryf::addtoroom(uint64_t roomid, eosio::name username, std::vector<uint64_t> heroes, uint64_t timestamp, uint8_t gametype)
{
    require_auth(MAINCONTRACT);

    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr == _usersingames.end(),"User is in game already.");

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(roomid);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");
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


//delete room, when game was ended or user return heroes
ACTION darkcountryf::deleteroom(uint64_t roomid)
{
    require_auth(MAINCONTRACT);

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(roomid);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");
    eosio::check((room_itr->status == 4)||(room_itr->username2 == eosio::name("wait")||(room_itr->status == 101)||(room_itr->status == 100)) ,"Game wasn't ended.");

    fights _fights(MAINCONTRACT,MAINCONTRACT.value);
    auto fight_itr = _fights.find(roomid);
    if(fight_itr != _fights.end())
    {
        _fights.erase(fight_itr);
    }

    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto itr = _usersingames.find(room_itr->username1.value);
    eosio::check(itr != _usersingames.end(),"User isn't in game.");

    _usersingames.erase(itr);

    _rooms.erase(room_itr);
}



////////////////////////////////////////////////////////////
//              FIGHT ACTIONS
////////////////////////////////////////////////////////////

//user turn: attack side, block  side, is special attack
ACTION darkcountryf::turn(eosio::name username, std::string attack, std::string blocktype, bool specialatk)
{
    (has_auth(username)) ? require_auth(username) : require_auth(MAINCONTRACT);

    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr != _usersingames.end(),"User is not playing game now.");

    missrounds _miss(MAINCONTRACT, MAINCONTRACT.value);
    auto miss_itr = _miss.find(username.value);
    if(miss_itr != _miss.end())
        _miss.erase(miss_itr);

    rooms _rooms(MAINCONTRACT,MAINCONTRACT.value);
    auto room_itr = _rooms.find(user_itr->roomid);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");
    eosio::check((room_itr->username1 == username)||(room_itr->username2 == username),"Current user is not playing in this room.");
    eosio::check((room_itr->status == 0)||(room_itr->status == 1),"Fight phase was ended.");

    if(room_itr->username1 == username)
    {    
        uint8_t pos = findhero(room_itr->heroes1);
        if(specialatk)
        eosio::check(room_itr->heroes1.at(pos).energy >=25,"Your hero hasn't enough energy for special attack");
    }
    else
    {
        uint8_t pos = findhero(room_itr->heroes2);
        if(specialatk)
        eosio::check(room_itr->heroes2.at(pos).energy >=25,"Your hero hasn't enough energy for special attack");
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
                eosio::check(0,"Something went wrong...");
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

                //clear fight data
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


                //clear fight data
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
                eosio::check(0,"Something went wrong...");
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


//if opponent doesn't turn for 30 sec, user, who did it, can end turn
ACTION darkcountryf::endturn(eosio::name username)
{
    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.find(username.value);
    eosio::check(user_itr != _usersingames.end(),"User isn't in game already.");

    rooms _rooms(MAINCONTRACT,MAINCONTRACT.value);
    auto room_itr = _rooms.find(user_itr->roomid);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");
    eosio::check((room_itr->username1 == username)||(room_itr->username2 == username),"Current user is not playing in this room.");
    eosio::check(room_itr->status == 1,"In this game phase you can't end turn.");
    eosio::check(room_itr->timestamp+30 <= (uint64_t)eosio::current_time_point().sec_since_epoch(),"You can end turn when opponent has not done his turn for 30 seconds.");

    fights _fights(MAINCONTRACT, MAINCONTRACT.value);
    auto fight_itr = _fights.find(user_itr->roomid);
    eosio::check(fight_itr != _fights.end(),"Fight wasn't found.");

    eosio::name missplayer;

    if(!fight_itr->firstuser.heroname.empty())
    {
        (has_auth(fight_itr->firstuser.username)) ? require_auth(fight_itr->firstuser.username) : require_auth(MAINCONTRACT);

        missplayer = room_itr->username2;
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

        (has_auth(fight_itr->seconduser.username)) ? require_auth(fight_itr->seconduser.username) : require_auth(MAINCONTRACT);

        missplayer = room_itr->username1;
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

    //add miss round to user, who miss current round
    uint8_t miss_rounds = 0;

    missrounds _miss(MAINCONTRACT, MAINCONTRACT.value);
    auto miss_itr = _miss.find(missplayer.value);
    if(miss_itr == _miss.end())
    {
        _miss.emplace(MAINCONTRACT, [&](auto& new_miss){
            new_miss.username = missplayer;
            new_miss.rounds = 1;
        });
        miss_rounds = 1;
    }
    else
    {
        _miss.modify(miss_itr, MAINCONTRACT, [&](auto& mod_miss){
            mod_miss.rounds++;
        });
        miss_rounds = miss_itr->rounds;
    }

    //end turn on room
    _rooms.modify(room_itr, MAINCONTRACT, [&](auto& mod_room){
        if(miss_rounds >= 5)
        {
            mod_room.status = 4; 
            if(missplayer == mod_room.username1)
            {
                mod_room.heroes1[0].health = 0;
                mod_room.heroes1[1].health = 0;
                mod_room.heroes1[2].health = 0;
            }
            else
            {
                mod_room.heroes2[0].health = 0;
                mod_room.heroes2[1].health = 0;
                mod_room.heroes2[2].health = 0;
            }
        }
        else
        {
            mod_room.status = 2;
        }
        mod_room.timestamp = eosio::current_time_point().sec_since_epoch();
    });
    //if user miss to much rounds -> end game
    if(miss_rounds >= 5)
    {
        _miss.erase(miss_itr);
        eosio::action logFight = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            MAINCONTRACT,
            eosio::name("endgame"),
            std::make_tuple(room_itr->roomid));
        logFight.send();
    }
}


//calculate fight data: take access to WAX rnd
ACTION darkcountryf::fight(uint64_t roomid, eosio::name username1, std::string attacktype1, std::string block1, 
                eosio::name username2, std::string attacktype2, std::string block2)
{
    require_auth(MAINCONTRACT);

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(roomid);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");
    eosio::check((room_itr->username1 == username1)&&(room_itr->username2 == username2),"One of users doesn't play in this room.");
    eosio::check(room_itr->status == 2,"Fight phase wasn't ended.");

    fights _fights(MAINCONTRACT, MAINCONTRACT.value);
    auto fight_itr = _fights.find(roomid);
    eosio::check(fight_itr != _fights.end(),"No active fights in this room.");
    eosio::check((fight_itr->firstuser.username == username1)&&(fight_itr->seconduser.username == username2),"One of usernames is wrong.");
    
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


//receive wax.rng random value
ACTION darkcountryf::receiverand (uint64_t customer_id, const eosio::checksum256 &random_value)
{
    require_auth(eosio::name("orng.wax"));

    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.find(customer_id);
    eosio::check(room_itr != _rooms.end(),"Room doesn't exist.");
    eosio::check(room_itr->status == 2,"Fight phase wasn't ended.");

    fights _fights(MAINCONTRACT, MAINCONTRACT.value);
    auto fight_itr = _fights.find(customer_id);
    eosio::check(fight_itr != _fights.end(),"No active fights in this room.");

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
    //set damage from every hero
    uint64_t damage1 = setdamage(num1, fight_itr->firstuser.attacktype, fight_itr->seconduser.blocktype, (bool)fight_itr->firstuser.energy, room_itr->heroes1.at(pos1).rarity);
    uint64_t damage2 = setdamage(num2, fight_itr->seconduser.attacktype, fight_itr->firstuser.blocktype, (bool)fight_itr->seconduser.energy, room_itr->heroes2.at(pos2).rarity);


    //modify hero data on room table
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

    //add user stats:kills and damage
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


//end game and create logs
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

    rateusers _rate(MAINCONTRACT, MAINCONTRACT.value);
    auto rate_itr = _rate.find(roomid);
    eosio::check(rate_itr == _rate.end(),"This room rate table was created.");

    double winaddrate, loseaddrate;

    int end_hero_points = win_hero_points - lose_hero_points;

    if(win_points > lose_points)
    {
        winaddrate = (double)(23-end_hero_points);
        if(lose_points < 1000)
            winaddrate = 1;
        eosio::action updatePoint = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, winner, winaddrate,std::string("")));
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
        
        loseaddrate = (double)(del_points+end_hero_points);

        eosio::action update1Point = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, loser, loseaddrate,std::string("")));
        update1Point.send();
    }
    else if (win_points < lose_points)
    {
        winaddrate = (double)(27-end_hero_points);
        eosio::action updatePoint = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, winner, winaddrate,std::string("")));
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

        loseaddrate = (double)(del_points+end_hero_points);

        eosio::action update1Point = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, loser, loseaddrate,std::string("")));
        update1Point.send();
    }
    else
    {
        winaddrate = (double)(0-end_hero_points);
        eosio::action updatePoint = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, winner, winaddrate,std::string("")));
        updatePoint.send();


        double del_points;
        if(lose_points >= 0.0)
        {
            del_points = 0;
        }
        else
        {
            del_points = lose_points;
        }

        loseaddrate = (double)(del_points+end_hero_points);
        
        eosio::action update1Point = eosio::action(
            eosio::permission_level{MAINCONTRACT, eosio::name("active")},
            NESTCONTRACT,
            eosio::name("update"),
            std::make_tuple((uint64_t)2, loser, loseaddrate,std::string("")));
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

    _rate.emplace(MAINCONTRACT, [&](auto& new_rate){
        new_rate.roomid = roomid;
        new_rate.winner = winner;
        new_rate.winrate = win_points;
        new_rate.winpoints = winaddrate;
        new_rate.loser = loser;
        new_rate.loserate = lose_points;
        new_rate.losepoints = loseaddrate;
    });

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
            rateusers _rate(MAINCONTRACT, MAINCONTRACT.value);
            auto rate_itr = _rate.find(room_itr->roomid);
            eosio::check(rate_itr != _rate.end(),"This room rate wasn't created.");

            _rate.erase(rate_itr);

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

    _rooms.erase(room_itr);
}

ACTION darkcountryf::cleanall()
{

    fights _fights(MAINCONTRACT, MAINCONTRACT.value);
    auto fight_itr = _fights.begin();
    while (fight_itr != _fights.end())
    {
        fight_itr = _fights.erase(fight_itr);
    }
    
    rooms _rooms(MAINCONTRACT, MAINCONTRACT.value);
    auto room_itr = _rooms.begin();
    while (room_itr != _rooms.end())
    {
        room_itr = _rooms.erase(room_itr);
    }
    
    usersingames _usersingames(MAINCONTRACT, MAINCONTRACT.value);
    auto user_itr = _usersingames.begin();
    while (user_itr != _usersingames.end())
    {
        user_itr = _usersingames.erase(user_itr);
    }
    
    heroes _heroes(MAINCONTRACT, MAINCONTRACT.value);
    auto hero_itr = _heroes.begin();
    while (hero_itr != _heroes.end())
    {
        hero_itr = _heroes.erase(hero_itr);
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
        base *= 1.6;
        (specialatk) ? base *= 2 : base *= 1;
        if((blocktype == "1")||(blocktype == "2"))
        {
            base *= 0.1;
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
                base *= 0.1;
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
                base *= 0.1;
            }
            return (uint64_t)base;
        }
    }
    else if(attacktype == "4")
    {   
        if(randnumb<_cstate.achest)
        {
            base *= 2.667;
            (specialatk) ? base *= 2 : base *= 1;
            if((blocktype == "4")||(blocktype == "5"))
            {
                base *= 0.1;
            }
            return (uint64_t)base;
        }
    }
    else if(attacktype == "5")
    {   
        if(randnumb<_cstate.ahead)
        {
            base *= 4.859;
            (specialatk) ? base *= 2 : base *= 1;
            if((blocktype == "1")||(blocktype == "5"))
            {
                base *= 0.1;
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
      EOSIO_DISPATCH_HELPER(darkcountryf, (useenergy)(resetstats)(createroom)(addtoroom)(deleteroom)(addheroes)(setchance)(returnheroes)(turn)(endturn)(fight)(receiverand)(cleanall)(delgame)(endgame)(deluser)(logfight)(cleanrooms)(deserialize)(endgamelog))
    }
  }
  else if (code == ATOMICCONTRACT.value && action == eosio::name("transfer").value)
  {
    execute_action(eosio::name(receiver), eosio::name(code), &darkcountryf::transferatom);
  }
    else if (code == eosio::name("eosio.token").value && action == eosio::name("transfer").value)
    {
        execute_action(eosio::name(receiver), eosio::name(code), &darkcountryf::transfer);
    }
  else
  {
    printf("Couldn't find an action.");
    return;
  }
}

