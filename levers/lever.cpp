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
	name   contract_name;
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
    void bullbuy(const name& from, const name& to,const asset& quantity, const string& memo) {
      
          if (from == get_self() || to != get_self() || from =="eosio.ram"_n||from =="eosio.stake"_n){return;}
          if( quantity.symbol != USN_SYMBOL){return;}  
          if(memo == "deposit"){return;}    
          check(quantity.is_valid(), "Invalid token transfer"); 
 
          auto tokenid = 99;
          if(memo == "0"){tokenid = 0;}
          if(memo == "1"){tokenid = 1;}
          if(memo == "2"){tokenid = 2;}
          if(memo == "3"){tokenid = 3;}
          if(memo == "4"){tokenid = 4;}
          if(memo == "5"){tokenid = 5;}    

          bull_index bulltable(get_self(), get_self().value);
          auto bull_itr = bulltable.find(tokenid);
          check(bull_itr != bulltable.end(),"bulltable have not find this tokenid");

          pair_index pairs("swap.defi"_n,"swap.defi"_n.value);
          auto iterator_BOX = pairs.find(bull_itr->defibox_tokenid);
          check(iterator_BOX != pairs.end(),"defibox pairs have not find this id");
          
          double reserve_EOS;
          double reserve_USN;
          if(iterator_BOX->reserve0.symbol == EOS_SYMBOL){
            reserve_EOS = (iterator_BOX->reserve0.amount)*0.0001;   //.amount 后类型变为uint64_t了,精度为4
            reserve_USN = (iterator_BOX->reserve1.amount)*0.0001;
          }else{
            reserve_EOS = (iterator_BOX->reserve1.amount)*0.0001;   //.amount 后类型变为uint64_t了,精度为4
            reserve_USN = (iterator_BOX->reserve0.amount)*0.0001;
          } 
          auto Price_EOSUSN = reserve_EOS/reserve_USN;        

          auto assetbalance = getbalance( bull_itr->contract_name, get_self(), bull_itr->token_symbol );

          double bulluintvalue;
          if(bull_itr->totalsupply ==0){
              bulluintvalue = 1;
          }else{
              bulluintvalue = (assetbalance.amount * Price_EOSUSN - bull_itr->debet)/bull_itr->totalsupply;
          }
          auto eosbull_amount = quantity.amount/bulluintvalue;
          auto borrow_amount = 2 * quantity.amount;

          bulltable.modify(bull_itr, get_self(), [&]( auto& row ) {    //modify( const T& obj, name payer, Lambda&& updater )                     
              row.totalsupply = row.totalsupply + eosbull_amount;
              row.debet = row.debet + borrow_amount;
          }); 


          //从借贷池借USN
          action(   //从stake池子借USN
              permission_level{ get_self(), name("active") },
              STAKE_CONTRACT, name("borrowtoken"),
              std::make_tuple(asset(borrow_amount,USN_SYMBOL))
          ).send();
          //.......//
          action(   //USN+借的USN---> defibox-swap --->EOS
              permission_level{ get_self(), name("active") },
              name("danchortoken"), name("transfer"),
              std::make_tuple(get_self(), BOX_CONTRACT, asset(3*quantity.amount,symbol("USN",4)),bull_itr->swap_memo)  // std::string("swap,0,9")
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
    void bullsell(const name& from, const name& to,const asset& quantity, const string& memo) {
      
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

    // '["0","eosio.token","4,EOS","0","0","9","swap,0,9"]' 
    [[eosio::action]]    
    void bullconfig(uint64_t id, name contract_name,symbol token_symbol, uint64_t totalsupply,uint64_t debet,uint64_t defibox_tokenid,std::string swap_memo) {
        require_auth( get_self() );
        bull_index bull_table(get_self(), get_self().value);
        auto iterator = bull_table.find(id);
        if( iterator == bull_table.end() )
        {
            bull_table.emplace(get_self(), [&]( auto& row ) {         //emplace( name payer, Lambda&& constructor ) 
            row.id = id;
            row.contract_name = contract_name;
            row.token_symbol = token_symbol;
            row.totalsupply = totalsupply;
            row.debet = debet;
            row.defibox_tokenid = defibox_tokenid;
            row.swap_memo = swap_memo;
            });
        }
        else {
            bull_table.modify(iterator, get_self(), [&]( auto& row ) {    //modify( const T& obj, name payer, Lambda&& updater )                     
            row.id = id;
            row.contract_name = contract_name;
            row.token_symbol = token_symbol;
            row.totalsupply = totalsupply;
            row.debet = debet;
            row.defibox_tokenid = defibox_tokenid;
            row.swap_memo = swap_memo;
            });
        }
    }
    [[eosio::action]]
    void erasebull(uint64_t id) {
      require_auth(get_self());
      bull_index result_table( get_self(), get_self().value ); 
      auto itr = result_table.find(id);
      check(itr != result_table.end(), "Record does not exist");
      result_table.erase(itr);
    }    
    [[eosio::action]]
    void erasebear(uint64_t id) {
      require_auth(get_self());
      bear_index result_table( get_self(), get_self().value ); 
      auto itr = result_table.find(id);
      check(itr != result_table.end(), "Record does not exist");
      result_table.erase(itr);
    } 


  private:

    struct [[eosio::table]] account {           
      asset balance;         
      uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };
    using account_index = eosio::multi_index<"accounts"_n,account>;

    asset getbalance(  const name& token_contract_account, const name& owner, const symbol& sym){
      account_index accountstable( token_contract_account, owner.value );
      const auto& ac = accountstable.get( sym.code().raw() );
      return ac.balance;
    }
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

    // //Lever_token表 --EOS, BOX ,PBTC 
    // struct [[eosio::table]] levertoken {
    //   uint64_t id;
    //   token token;
    //   std::string swap_memo;
    //   uint64_t primary_key() const { return id; }
    // };
    // using levertoken_index = eosio::multi_index<"levertokens"_n, levertoken>;


    //BULL资产负债表--EOS, BOX ,PBTC  0,1,2
    struct [[eosio::table]] bull {
      uint64_t id;
	    name   contract_name;
      symbol token_symbol;
      uint64_t totalsupply;
      uint64_t debet;
      //double bull_unitvalue;
      uint64_t defibox_tokenid;
      std::string swap_memo;      //"swap,0,9"
      uint64_t primary_key() const { return id; }
    };
    using bull_index = eosio::multi_index<"bulls"_n, bull>;

    //BEAR资产负债表--EOS, BOX ,PBTC  0,1,2
    struct [[eosio::table]] bear {
      uint64_t id;
	    name   contract_name;
      symbol token_symbol;
      uint64_t totalsupply;
      uint64_t debet;
      //double bear_unitvalue;
      uint64_t defibox_tokenid;
      std::string swap_memo;
      uint64_t primary_key() const { return id; }
    };
    using bear_index = eosio::multi_index<"bears"_n, bear>;

  };



