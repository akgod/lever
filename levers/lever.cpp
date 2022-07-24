// #include "lever.hpp"
#include <string>
#include <eosio/eosio.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio/transaction.hpp>
#include <math.h>

using namespace eosio;
using namespace std;

#define EOS_CONTRACT "eosio.token"_n
#define BOX_CONTRACT "swap.defi"_n
#define USN_CONTRACT "danchortoken"_n
#define TOKEN_CONTRACT "biceshi.max"_n
#define LEVER_CONTRACT "levtest.max"_n
#define STAKE_CONTRACT "statest.max"_n

#define EOS_SYMBOL symbol("EOS",4)
#define USN_SYMBOL symbol("USN",4)
#define EOSBULL_SYMBOL symbol("EOSBULL",4)
#define EOSBEAR_SYMBOL symbol("EOSBEAR",4)


struct token {
	name   contrat_name;
  symbol token_symbol;
};


class [[eosio::contract("lever")]] lever : public eosio::contract {

  public:     
    using contract::contract;
    void notify(name user, std::string msg) {
      require_auth(get_self());
      require_recipient(user);
    }
    [[eosio::on_notify("danchortoken::transfer")]]
    void usnbuy(const name& from, const name& to,const asset& quantity, const string& memo) {
      
          if (from == get_self() || to != get_self() || from =="eosio.ram"_n||from =="eosio.stake"_n){return;}
          if( quantity.symbol != USN_SYMBOL){return;}  
          if(memo == "deposit"){return;}    
          check(quantity.is_valid(), "Invalid token transfer"); 
          auto eosbull_amount = 3 * quantity.amount;
          auto borrow_amout = 2 * quantity.amount;
          //从借贷池借USN
          action(   //从stake池子借USN
              permission_level{ get_self(), name("active") },
              STAKE_CONTRACT, name("borrowtoken"),
              std::make_tuple(asset(borrow_amout,USN_SYMBOL))
          ).send();
          //.......//
          action(   //USN+借的USN---> defibox-swap --->EOS
              permission_level{ get_self(), name("active") },
              name("danchortoken"), name("transfer"),
              std::make_tuple(get_self(), BOX_CONTRACT, asset(eosbull_amount,symbol("USN",4)), std::string("swap,0,9"))
          ).send();

          action(  //issue 'bull/bear' token
              permission_level{ get_self(), name("active") },
              TOKEN_CONTRACT, name("issue"),
              std::make_tuple(get_self(), asset(eosbull_amount,EOSBULL_SYMBOL), std::string("ISSUE EOSBULL TOKEN"))
          ).send();
            
          action(   //transfer the eosbull/eosbear token to the buyer
              permission_level{ get_self(), name("active") },
              TOKEN_CONTRACT, name("transfer"),
              std::make_tuple(get_self(), from, asset(eosbull_amount,EOSBULL_SYMBOL),std::string("lever token"))
          ).send();
           
          //
          //
    }
    [[eosio::on_notify("biceshi.max::transfer")]]
    void eosbullsell(const name& from, const name& to,const asset& quantity, const string& memo) {
      
          if (from == get_self() || to != get_self() || from =="eosio.ram"_n||from =="eosio.stake"_n){return;}
          if( quantity.symbol != EOSBULL_SYMBOL){return;}  
          if(memo == "deposit"){return;}    
          check(quantity.is_valid(), "Invalid token transfer"); 
          auto eosbull_amount = 8899;
          
          action(  //retire 'bull/bear' token
              permission_level{ get_self(), name("active") },
              TOKEN_CONTRACT, name("retire"),
              std::make_tuple(asset(eosbull_amount,EOSBULL_SYMBOL), std::string("RETIRE EOSBULL TOKEN"))
          ).send();
    }
    


  private:
    void swaplog(name log_address, std::string message){
        require_auth( get_self() );
        action(
            permission_level{get_self(),"active"_n},
            get_self(),
            "notify"_n,
            std::make_tuple(log_address,message)
        ).send();
    }
  //deifbox的交易对表
    struct [[eosio::table]] pair {
      uint64_t id;
      token token0;
      token token1;
      asset reserve0;
      asset reserve1;
      uint64_t liquidity_token;
      double price0_last;
      double price1_last;
      uint64_t price0_cumulative_last;
      uint64_t price1_cumulative_last;
      time_point_sec block_time_last;
      uint64_t primary_key() const { return id; }
    };
    using pair_index = eosio::multi_index<"pairs"_n, pair>;

    //Lever_token表 --EOS, BOX ,PBTC 
    struct [[eosio::table]] levertoken {
      uint64_t id;
      token token;
      std::string swap_memo;
      time_point_sec block_time_last;
      uint64_t primary_key() const { return id; }
    };
    using levertoken_index = eosio::multi_index<"levertokens"_n, levertoken>;


    //BULL资产负债表
    struct [[eosio::table]] bull {
      uint64_t id;
      double totalsupply;
      double asset;
      double debet;
      double bull_unitvalue;
      time_point_sec block_time_last;
      uint64_t primary_key() const { return id; }
    };
    using bull_index = eosio::multi_index<"bulls"_n, bull>;

    //BEAR资产负债表
    struct [[eosio::table]] bear {
      uint64_t id;
      double totalsupply;
      double asset;
      double debet;
      double bear_unitvalue;
      time_point_sec block_time_last;
      uint64_t primary_key() const { return id; }
    };
    using bear_index = eosio::multi_index<"bears"_n, bear>;


  };



