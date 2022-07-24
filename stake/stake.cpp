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
#define EOSS_SYMBOL symbol("EOSS",4)
#define USNS_SYMBOL symbol("USNS",4)

struct token {
  name   contrat_name;
  symbol token_symbol;
};


class [[eosio::contract("stake")]] stake : public eosio::contract {

  public:     
    using contract::contract;
    void notify(name user, std::string msg) {
      require_auth(get_self());
      require_recipient(user);
    }
    [[eosio::on_notify("danchortoken::transfer")]]
    void deposit(const name& from, const name& to,const asset& quantity, const string& memo) {
      
          if (from == get_self() || to != get_self() || from =="eosio.ram"_n||from =="eosio.stake"_n){return;}
          if( quantity.symbol != USN_SYMBOL){return;}  
          //if(memo == "deposit"){return;}    
          check(quantity.is_valid(), "Invalid token transfer"); 

          sharetoken_index tokentable(get_self(), get_self().value);
          for(auto itr = tokentable.begin();itr != tokentable.end();itr++){            
            if(itr->token_symbol == quantity.symbol){              
                tokentable.modify(itr, get_self(), [&]( auto& row ) {    //modify( const T& obj, name payer, Lambda&& updater )                     
                  row.totaldeposit = row.totaldeposit + quantity.amount;
                  row.sharetoken_num = row.sharetoken_num + quantity.amount;
                  row.block_time_last = time_point_sec();
                });            
                action(  //issue the share token
                    permission_level{ get_self(), name("active") },
                    TOKEN_CONTRACT, name("issue"),
                    std::make_tuple(get_self(), asset(quantity.amount,USNS_SYMBOL), std::string("ISSUE SHARE TOKEN"))
                ).send();
                  
                action(   //transfer the share token to the depositor
                    permission_level{ get_self(), name("active") },
                    TOKEN_CONTRACT, name("transfer"),
                    std::make_tuple(get_self(), from, asset(quantity.amount,USNS_SYMBOL),std::string("TRANSFER TOKEN"))
                ).send();
            }
          }   
     
    }


    [[eosio::on_notify("biceshi.max::transfer")]]
    void withdraw(const name& from, const name& to,const asset& quantity, const string& memo){
        if (from == get_self() || to != get_self() || from =="eosio.ram"_n||from =="eosio.stake"_n){return;}
        //if( quantity.symbol != USNS_SYMBOL&& quantity.symbol != EOSS_SYMBOL){return;}  
        if(memo != "redeem"){return;}    
        check(quantity.is_valid(), "Invalid token transfer"); 
        sharetoken_index tokentable(get_self(), get_self().value );
        for(auto itr = tokentable.begin();itr != tokentable.end();itr++){
          if(quantity.symbol == itr->sharetoken_symbol){

            auto withdraw_amount = quantity.amount * itr->totaldeposit/itr->sharetoken_num;

            tokentable.modify(itr, get_self(), [&]( auto& row ) {    //modify( const T& obj, name payer, Lambda&& updater )                     
                row.totaldeposit = row.totaldeposit - quantity.amount;
                row.sharetoken_num = row.sharetoken_num - quantity.amount;
                row.block_time_last = time_point_sec();
            }); 

            action(  //retire 'USNS/EOSS' token
              permission_level{ get_self(), name("active") },
              TOKEN_CONTRACT, name("retire"),
              std::make_tuple(asset(quantity.amount,itr->sharetoken_symbol), std::string("RETIRE  TOKEN"))
            ).send();
            action(   //transfer the deposit token to the depositor
                permission_level{ get_self(), name("active") },
                itr->token_contract_address, name("transfer"),
                std::make_tuple(get_self(), from, asset(withdraw_amount,itr->token_symbol),std::string("token"))
            ).send(); 
          }
        }
    }

    // '["0","4,EOS","4,EOSS","0.0001","eosio.token"]'
        // '["1","4,USN","4,USNS","0.0001","danchortoken"]'
            // '["2","4,BOX","4,BOXS","0.0001","token.defi"]'
    [[eosio::action]]    
    void tokenconfig(uint64_t id, symbol token_symbol, symbol sharetoken_symbol,double token_precision,name token_contract_address) {
        require_auth( get_self() );
        sharetoken_index token_table(get_self(), get_self().value);
        auto iterator = token_table.find(id);
        if( iterator == token_table.end() )
        {
            token_table.emplace(get_self(), [&]( auto& row ) {         //emplace( name payer, Lambda&& constructor ) 
            row.id = id;        
            row.token_symbol = token_symbol;
            row.sharetoken_symbol = sharetoken_symbol;
            row.token_precision = token_precision;
            row.token_contract_address = token_contract_address;   
            row.totaldeposit = 0;
            row.sharetoken_num = 0;
            row.block_time_last = time_point_sec();
            });
        }
        else {
            token_table.modify(iterator, get_self(), [&]( auto& row ) {    //modify( const T& obj, name payer, Lambda&& updater )                     
            row.id = id;
            row.token_symbol = token_symbol;
            row.sharetoken_symbol = sharetoken_symbol;
            row.token_precision = token_precision;
            row.token_contract_address = token_contract_address;   
            row.totaldeposit = 0;
            row.sharetoken_num = 0;
            row.block_time_last = time_point_sec();
            });
        }
    }
    
    [[eosio::action]]
    void eraseall(uint64_t tab) {
      require_auth(_self);
      if(tab == 0){
          sharetoken_index result_table(get_self(), get_self().value);
          for (auto clean = result_table.begin(); clean != result_table.end();){         // erase table,清空result
            clean = result_table.erase(clean);
          } 
      }
    }
    [[eosio::action]]
    void erasedata(uint64_t sharetoken_id) {
      require_auth(get_self());
      sharetoken_index result_table( get_self(), get_self().value ); 
      auto itr = result_table.find( sharetoken_id);
      check(itr != result_table.end(), "Record does not exist");
      result_table.erase(itr);
    }

    [[eosio::action]]  
    void borrowtoken(const asset& quantity){
        require_auth( LEVER_CONTRACT );
        if(quantity.symbol == USN_SYMBOL){
          action(   //transfer the token to the borrower
            permission_level{ get_self(), name("active") },
            USN_CONTRACT, name("transfer"),
            std::make_tuple(get_self(), LEVER_CONTRACT, quantity,std::string("deposit"))
          ).send();
        };
        if(quantity.symbol == EOS_SYMBOL){
          action(   //transfer the token to the borrower
            permission_level{ get_self(), name("active") },
            EOS_CONTRACT, name("transfer"),
            std::make_tuple(get_self(), LEVER_CONTRACT, quantity,std::string("deposit"))
          ).send();
        };

    }

    [[eosio::action]]  
    void repaytoken(const asset& quantity){
        require_auth( LEVER_CONTRACT );

    }

  private:
    //借贷池--币 存款信息表
    struct [[eosio::table]] sharetoken {
      uint64_t id;
      symbol token_symbol;
      symbol sharetoken_symbol;
      double token_precision;
      name token_contract_address;      
      double totaldeposit;
      double sharetoken_num;
      time_point_sec block_time_last;
      uint64_t primary_key() const { return id; }
    };
    using sharetoken_index = eosio::multi_index<"sharetokens"_n, sharetoken>;
  };



