#pragma once
#include <algorithm>
#include <initializer_list>

#include <eosio/eosio.hpp>
#include <eosio/time.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>

#include <optional>

#include <eosio.token/eosio.token.hpp>
#include <../external/vdexdposvote/include/vdexdposvote/vdexdposvote.hpp>


using namespace eosio;
using namespace std;

static constexpr uint64_t DAY = 86400; // 24 hours
static constexpr uint64_t WEEK = 604800; // 7 days
static constexpr uint64_t MONTH = 2592000; // 30 days
static constexpr symbol CORE_SYMBOL = symbol{"VTX", 8};
static constexpr symbol BUDGET_SYMBOL = CORE_SYMBOL;
static constexpr name BUDGET_TOKEN_CONTRACT = "volentixgsys"_n;


// set TESTING true/false
static constexpr bool TESTING = true;



namespace eosio {

class [[eosio::contract("volentixwork")]] wps : public contract {


public:
    using contract::contract;

    /**
     * Construct a new contract given the contract name
     *
     * @param {name} receiver - The name of this contract
     * @param {name} code - The code name of the action this contract is processing.
     * @param {datastream} ds - The datastream used
     */
    wps( name receiver, name code, eosio::datastream<const char*> ds )
        : contract( receiver, code, ds ),
            _proposals( _self, _self.value ),
            _votes( _self, _self.value ),
            _settings( _self, _self.value ),
            _state( _self, _self.value ),
            _deposits( _self, _self.value ),
            _proposers( _self, _self.value ),
            _periods( _self, _self.value ),
            _claims( _self, _self.value )
    {}


    /**
 * ## TABLE `settings`
 *
 * - `{int16_t} [vote_margin=20]` - minimum BP vote margin threshold to reach for proposals
 * - `{asset} [deposit_required="100.0000 VTX"]` - deposit required to active proposal
 * - `{uint64_t} [voting_interval=2592000]` - voting interval in seconds
 * - `{asset} [max_monthly_budget="25000.0000 VTX"]` - maximum monthly budget
 * - `{uint64_t} [min_time_voting_end=432000]` - minimum time required to activate at the end of the current voting period
 *
 * ### example
 *
 * ```json
 * {
 *   "vote_margin": 20,
 *   "deposit_required": "100.0000 VTX",
 *   "voting_interval": 2592000,
 *   "max_monthly_budget": "25000.0000 VTX",
 *   "min_time_voting_end": 432000
 * }
 * ```
 */
struct [[eosio::table("settings"), eosio::contract("volentixwork")]] wps_parameters {
    int16_t                 vote_margin = 20;
    asset                   deposit_required = asset{ 1000000, BUDGET_SYMBOL};
    uint64_t                voting_interval = MONTH;
    asset                   max_monthly_budget = asset{ 250000000, BUDGET_SYMBOL};
    uint64_t                min_time_voting_end = DAY * 5;
};

typedef eosio::singleton< "settings"_n, wps_parameters> settings_table;







    /**
     * ## ACTION `submitdraft`
     *
     * Submit a WPS proposal
     *
     * - **authority**: `proposer`
     * - **ram_payer**: `proposer`
     *
     * ### params
     *
     * - `{name} proposer` - proposer of proposal
     * - `{name} proposal_name` - proposal name
     * - `{string} title` - proposal title
     * - `{asset} monthly_budget` - monthly budget payment request
     * - `{uin8_t} duration` - monthly budget duration (maximum of 6 months)
     * - `{map<name, string>} proposal_json` - a sorted container of <key, value>
     *
     * ### example
     *
     * ```bash
     * cleos push action volentixwork submitdraft '["myaccount", "mywps", "My WPS", "500.0000 VTX", 1, [{"key":"category", "value":"other"}, {"key":"region", "value":"global"}]]' -p myaccount
     * ```
     */

    [[eosio::action]]
    void submitdraft( const name proposer,
                      const name proposal_name,
                      const string title,
                      const uint8_t type,
                      const uint8_t security,
                      const uint8_t impact,
                      const asset monthly_budget,
                      const uint8_t duration,
                      const map<name, string> proposal_json );

    /**
     * ## ACTION `vote`
     *
     * Vote for a WPS proposal
     *
     * - **authority**: `voter`
     * - **ram_payer**: `_self`
     *
     * ### params
     *
     * - `{name} voter` - voter
     * - `{name} proposal_name` - proposal name
     * - `{name} vote` - vote (yes/no/abstain)
     *
     * ```bash
     * cleos push action volentixwork vote '["myaccount", "mywps", "yes"]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void vote( const name voter, const name proposal_name, const name vote );

    /**
     * ## ACTION `activate`
     *
     * Activate WPS proposal at a specified voting period
     *
     * - **authority**: `proposer`
     * - **ram_payer**: `_self`
     *
     * ### params
     *
     * - `{name} proposer` - proposer
     * - `{name} proposal_name` - proposal name
     * - `{time_point_sec} [start_voting_period=null]` - (optional) activate proposal at the specified voting period (must be current or next)
     *
     * ```bash
     * cleos push action volentixwork activate '["myaccount", "mywps", "2019-11-25T00:00:00"]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void activate( const name proposer, const name proposal_name, const bool activate_next );

    /**
     * ## ACTION `refund`
     *
     * Refund any remaining deposit amount from requesting account
     *
     * - **authority**: `account`
     *
     * ### params
     *
     * - `{name} account` - account requesting refund
     *
     * ```bash
     * cleos push action volentixwork refund '["myaccount"]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void refund( const name account );

    /**
     * ## ACTION `canceldraft`
     *
     * Cancel draft WPS proposal
     *
     * - **authority**: `proposer`
     *
     * ### params
     *
     * - `{name} proposer` - proposer
     * - `{name} proposal_name` - proposal name
     *
     * ```bash
     * cleos push action volentixwork canceldraft '["myaccount", "mywps"]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void canceldraft( const name proposer, const name proposal_name );

    /**
     * ## ACTION `modifydraft`
     *
     * Modify draft WPS proposal
     *
     * - **authority**: `proposer`
     * - **ram_payer**: `proposer`
     *
     * ### params
     *
     * - `{name} proposer` - proposer of proposal
     * - `{name} proposal_name` - proposal name
     * - `{string} title` - proposal title
     * - `{map<name, string>} proposal_json` - a sorted container of <key, value>
     *
     * ### example
     *
     * ```bash
     * cleos push action volentixwork modifydraft '["myaccount", "mywps", "My WPS", [{"key":"region", "value":"global"}]]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void modifydraft(const name proposer, const name proposal_name, const string title, const map<name, string> proposal_json );

    /**
     * ## ACTION `modifybudget`
     *
     * Modify draft WPS proposal budget
     *
     * - **authority**:  `proposer`
     * - **ram_payer**: `proposer`
     *
     * ### params
     *
     * - `{name} proposer` - proposer of proposal
     * - `{name} proposal_name` - proposal name
     * - `{asset} monthly_budget` - monthly budget payment request
     * - `{uin8_t} duration` - monthly budget duration (maximum of 6 months)
     *
     * ### example
     *
     * ```bash
     * cleos push action volentixwork modifybudget '["myaccount", "mywps", "500.0000 VTX", 2]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void modifybudget(const name proposer, const name proposal_name, const asset monthly_budget, const uint8_t duration );

    /**
     * ## ACTION `setproposer`
     *
     * Set proposer's metadata
     *
     * - **authority**: `proposer`
     * - **ram_payer**: `proposer`
     *
     * ### params
     *
     * - `{name} proposer` - proposer of proposal
     * - `{map<name, string>} metadata_json` - a sorted container of <key, value>
     *
     * ### example
     *
     * ```bash
     * cleos push action volentixwork setproposer '["myaccount", [{"key":"region", value":"global"}]]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void rmproposer(const name proposer );
    /**
     * ## ACTION `rmproposer`
     *
     * Rrmove proposer
     *
     * - **authority**: `proposer`
     * - **ram_payer**: `proposer`
     *
     * ### params
     *
     * - `{name} proposer` - proposer of proposal
   
     *
     * ### example
     *
     * ```bash
     * cleos push action volentixwork rmproposer '["myaccount"]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void setproposer(const name proposer, const map<name, string> proposer_json );

    /**
     * ## ACTION `wipe deposits`
     *
     * Initialize WPS contract
     *
     * - **authority**: `_self`
     * - **ram_payer**: `_self`
     *
     * ### params
     *
     * 
     * ```bash
     * cleos push action volentixwork set proposer '[]' -p volentixwork
     * ```
     */
    [[eosio::action]]
    void wipedeposits( const eosio::name account);
    
    /**
     * ## ACTION `init`
     *
     * Initialize WPS contract
     *
     * - **authority**: `_self`
     * - **ram_payer**: `_self`
     *
     * ### params
     *
     * - `{wps_parameters} params` - VTX WPS parameters
     *
     * ```bash
     * 
     * ```
     */
    [[eosio::action]]
    void init( const wps_parameters params );

    /**
     * ## ACTION `setparams`
     *
     * Set paramaters for WPS contract
     *
     * - **authority**: `_self`
     * - **ram_payer**: `_self`
     *
     * ### params
     *
     * - `{wps_parameters} params` - VTX WPS parameters
     *
     * ```bash
     * cleos push action volentixwork setparams '[{"vote_margin": 20, "deposit_required": "100.0000 VTX", "voting_interval": 2592000, "max_monthly_budget": "25000.0000 VTX", "min_time_voting_end": 432000 }]' -p volentixwork
     * ```
     */
    [[eosio::action]]
    void setparams( const wps_parameters params );
    
    /**
     * ## ACTION `complete`
     *
     * Complete WPS voting period
     *
     * - **authority**: `any`
     *
     * ### params
     *
     * N/A
     *
     * ```bash
     * cleos push action volentixwork complete '[]' -p volentixwork
     * ```
     */
    [[eosio::action]]
    void complete( );

    /**
     * ## ACTION `claim`
     *
     * Claim remaining proposal amount, transfer amount to proposer
     *
     * - **authority**: `any`
     *
     * ### params
     *
     * - `{name} proposal_name` - proposal name to claim
     *
     * ```bash
     * cleos push action volentixwork claim '["mywps"]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void claim( const name proposal_name );

    /**
     * ## ACTION `refresh`
     *
     * Update `votes` from eligible voters
     * Any existing votes with voters with less than 100 VTX vpay will be removed
     *
     * - **authority**: `any`
     *
     * ```bash
     * cleos push action volentixwork refresh '[]' -p myaccount
     * ```
     */
    [[eosio::action]]
    void refresh( );

    [[eosio::action]]
    void rmproposal( const eosio::name proposer, const eosio::name proposal_name);

    [[eosio::action]]
    void clean() {
        require_auth( _self );

        _state.remove();
        _settings.remove();

        auto it = _proposals.begin();
        while (it != _proposals.end()) {
            it = _proposals.erase(it);
        }

        while (_votes.begin() != _votes.end()) {
            _votes.erase(_votes.begin());
        }

        while (_deposits.begin() != _deposits.end()) {
            _deposits.erase(_deposits.begin());
        }

        while (_proposers.begin() != _proposers.end()) {
            _proposers.erase(_proposers.begin());
        }

        while (_periods.begin() != _periods.end()) {
            _periods.erase(_periods.begin());
        }

        while (_claims.begin() != _claims.end()) {
            _claims.erase(_claims.begin());
        }

        // TODO Comments ?
    }

    [[eosio::action]]
    void comment( const name account, const name proposal_name, const map<name, string> comment_json );

    [[eosio::on_notify("volentixgsys::transfer")]]
    void transfer( const name&    from,
                   const name&    to,
                   const asset&   quantity,
                   const string&  memo );

    using vote_action = eosio::action_wrapper<"vote"_n, &wps::vote>;
    using submitdraft_action = eosio::action_wrapper<"submitdraft"_n, &wps::submitdraft>;
    using activate_action = eosio::action_wrapper<"activate"_n, &wps::activate>;
    using refund_action = eosio::action_wrapper<"refund"_n, &wps::refund>;
    using canceldraft_action = eosio::action_wrapper<"canceldraft"_n, &wps::canceldraft>;
    using modifydraft_action = eosio::action_wrapper<"modifydraft"_n, &wps::modifydraft>;
    using init_action = eosio::action_wrapper<"init"_n, &wps::init>;
    using setparams_action = eosio::action_wrapper<"setparams"_n, &wps::setparams>;
    using setproposer_action = eosio::action_wrapper<"setproposer"_n, &wps::setproposer>;
    using complete_action = eosio::action_wrapper<"complete"_n, &wps::complete>;
    using claim_action = eosio::action_wrapper<"claim"_n, &wps::claim>;
    using refresh_action = eosio::action_wrapper<"refresh"_n, &wps::refresh>;
    using comment_action = eosio::action_wrapper<"comment"_n, &wps::comment>;

    using clean_action = eosio::action_wrapper<"clean"_n, &wps::clean>;
    using rmproposer_action = eosio::action_wrapper<"rmproposer"_n, &wps::rmproposer>;
    using rmproposal_action = eosio::action_wrapper<"rmproposal"_n, &wps::rmproposal>;
    using wipedeposits_action = eosio::action_wrapper<"wipedeposits"_n, &wps::wipedeposits>;

private:

    // private helpers
    // ===============

    // periods
    void add_proposal_to_periods( const name proposal_name, const time_point_sec voting_period );
    void copy_active_voting_periods( const time_point_sec current_voting_period, const time_point_sec next_voting_period );
    void check_max_number_proposals();

    // activate
    void check_min_time_voting_end( );
    void check_draft_proposal_exists( const name proposer, const name proposal_name );
    void deduct_proposal_activate_fee( const name proposer );
    void emplace_proposal_from_draft( const name proposer, const name proposal_name, const bool activate_next );
    void emplace_empty_votes( const name proposal_name );
    void check_eligible_proposer( const name proposer );

    // vote
    bool is_min_vote_margin(const int16_t& total_net_votes, const asset& monthly_budget); // TODO More parameters here
    int16_t calculate_total_net_votes( const map<name, name> votes, const set<name> eligible_producers );
    bool update_total_net_votes( const name proposal_name, const std::map<name, name> votes, const set<name> eligible_producers );
    void update_vote( const name voter, const name proposal_name, const name vote, const set<name> eligible_producers );
    void update_eligible_proposals( );
    void check_proposal_can_vote( const name proposal_name );
    void check_voter_eligible( const name voter );
    map<int16_t, set<name>> sort_proposals_by_net_votes( const name status );

    // utils
    checksum256 get_tx_id();

    // complete
    void check_voting_period_completed();
    void set_pending_to_active();
    void handle_payouts();
    bool proposal_exists_per_voting_period( const name proposal_name, const time_point_sec voting_period );
    void update_proposal_status( const name proposal_name );
    void update_to_next_voting_period();
    bool is_voting_period_complete();
    set<name> group_proposals( const name status );

    // claims
    void add_claim( const name proposer, const name proposal_name, const asset quantity );

    // settings
    void add_funding( const asset quantity );
    void sub_funding( const asset quantity );
    void check_contract_active();
    void check_wps_parameters( const wps_parameters params );

    // deposits
    void add_deposit( const name account, const asset quantity, const name ram_payer );
    void sub_deposit( const name account, const asset quantity );
    void create_deposit_account( const name account, const name ram_payer );
    void add_liquid_deposits( const asset quantity );
    void sub_liquid_deposits( const asset quantity );
    void deposit_to_proposal( const name proposal_name, const asset quantity );
    void move_to_locked_deposits( const asset quantity );

    // drafts
    void check_title( const string title );
    void check_duration( const uint8_t duration );
    void check_monthly_budget( const asset monthly_budget);

    // start
    void check_available_funding();

    // refresh
    bool refresh_proposals();
    bool refresh_proposal( const name proposal_name, const set<name> eligible_producers );
    bool is_voter_eligible( const name voter );
    void get_all_voter_eligible();
    set<name> get_eligible_producers();
    /**
 * ## TABLE `drafts`
 *
 * - scope: `proposer`
 *
 * - `{name} proposer` - proposer of proposal
 * - `{name} proposal_name` - proposal name
 * - `{string} title` - proposal title
 * - `{asset} monthly_budget` - monthly budget payment request
 * - `{uint8_t} duration` - monthly budget duration (maximum of 6 months)
 * - `{asset} total_budget` - total budget payment request
 * - `{map<name, string>} proposal_json` - a sorted container of <key, value>
 *
 * ### example
 *
 * ```json
 * {
 *   "proposer": "myaccount",
 *   "proposal_name": "mywps",
 *   "title": "My WPS",
 *   "monthly_budget": "500.0000 VTX",
 *   "duration": 2,
 *   "total_budget": "1000.0000 VTX",
 *   "proposal_json": [
 *     { "key": "category", "value": "other" },
 *     { "key": "region", "value": "global" }
 *   ]
 * }
 * ```
 */
/*****************************************/
struct [[eosio::table("drafts"), eosio::contract("volentixwork")]] drafts_row {
    name                        proposer;
    name                        proposal_name;
    string                      title;
    asset                       monthly_budget;
    uint8_t                     type;
    uint8_t                     security;
    uint8_t                     impact;
    uint8_t                     duration;
    asset                       total_budget;
    map<name, string>           proposal_json;

    uint64_t primary_key() const { return proposal_name.value; }
};
typedef eosio::multi_index< "drafts"_n, drafts_row> drafts_table;


/**
 * ## TABLE `proposers`
 *
 * - `{name} proposer` - proposer of proposal
 * - `{map<name, string>} metadata_json` - a sorted container of <key, value>
 *
 * ### example
 *
 * ```json
 * {
 *   "proposer": "myaccount",
 *   "metadata_json": [
 *     { "key": "region", "value": "global" }
 *   ]
 * }
 * ```
 */
struct [[eosio::table("proposers"), eosio::contract("volentixwork")]] proposers_row {
    name                    proposer;
    map<name, string>       proposer_json;

    uint64_t primary_key() const { return proposer.value; }
};
typedef eosio::multi_index< "proposers"_n, proposers_row> proposers_table;

/**
 * ## TABLE `proposals`
 *
 * - `{name} proposer` - proposer of proposal
 * - `{name} proposal_name` - proposal name
 * - `{string} title` - proposal title
 * - `{asset} monthly_budget` - monthly budget payment request
 * - `{uint8_t} duration` - monthly budget duration (maximum of 6 months)
 * - `{asset} total_budget` - total budget payment request
 * - `{map<name, string>} proposal_json` - a sorted container of <key, value>
 * - `{name} status` - status of proposal (active/expired/completed)
 * - `{int16_t} total_net_votes` - total net votes
 * - `{bool} eligible` - (true/false) eligible for current voting period payout
 * - `{asset} payouts` - total payouts amount received
 * - `{asset} claimed` - total claimed amount
 * - `{time_point_sec} created` - time proposal was created (UTC)
 * - `{time_point_sec} start_voting_period` - start of voting period (UTC)
 * - `{time_point_sec} end` - end of proposal (UTC)
 *
 * ### example
 *
 * ```json
 * {
 *   "proposer": "myaccount",
 *   "proposal_name": "mywps",
 *   "title": "My WPS",
 *   "monthly_budget": "500.0000 VTX",
 *   "duration": 2,
 *   "total_budget": "1000.0000 VTX",
 *   "proposal_json": [
 *     { "key": "category", "value": "other" },
 *     { "key": "region", "value": "global" }
 *   ],
 *   "status": "active",
 *   "total_net_votes": 2,
 *   "eligible": false,
 *   "payouts": "0.0000 VTX",
 *   "claimed": "0.0000 VTX",
 *   "created": "2019-11-05T12:10:00",
 *   "start_voting_period": "2019-11-01T00:00:00",
 *   "end": "2019-12-01T00:00:00"
 * }
 * ```
 */

struct [[eosio::table("comments"), eosio::contract("volentixwork")]] comments_row {
    name                    account;
    name                    account_type;
    time_point_sec          timestamp;
    uint16_t                version;
    map<name, string>       comment_json;

    uint64_t primary_key() const { return account.value; }
    uint64_t by_timestamp() const { return timestamp.sec_since_epoch(); }
    uint64_t by_account_type() const { return account_type.value; }
};
typedef eosio::multi_index< "comments"_n, comments_row,
    indexed_by<"bytimestamp"_n, const_mem_fun<comments_row, uint64_t, &comments_row::by_timestamp>>,
    indexed_by<"byaccnttype"_n, const_mem_fun<comments_row, uint64_t, &comments_row::by_account_type>>
> comments_table;


struct [[eosio::table("proposals"), eosio::contract("volentixwork")]] proposals_row : drafts_row {
    // inherent fields from `drafts` TABLE
    name                    status;
    int16_t                 total_net_votes;
    bool                    eligible;
    asset                   payouts;
    asset                   claimed;
    time_point_sec          created;
    time_point_sec          start_voting_period;
    int16_t                 remaining_voting_periods;

    uint64_t primary_key() const { return proposal_name.value; }
    uint64_t by_status() const { return status.value; }
    uint64_t by_proposer() const { return proposer.value; }
};

typedef eosio::multi_index< "proposals"_n, proposals_row,
    indexed_by<"bystatus"_n, const_mem_fun<proposals_row, uint64_t, &proposals_row::by_status>>,
    indexed_by<"byproposer"_n, const_mem_fun<proposals_row, uint64_t, &proposals_row::by_proposer>>
> proposals_table;

/**
 * ## TABLE `votes`
 *
 * - `{name} proposal_name` - The proposal's name, its ID among all proposals
 * - `{map<name, name>} votes` - a sorted container of <voter, vote>
 *
 * ### example
 *
 * ```json
 * {
 *   "proposal_name": "mywps",
 *   "votes": [
 *      { "key": "mybp1", "value": "yes" },
 *      { "key": "mybp2", "value": "no" },
 *      { "key": "mybp3", "value": "yes" },
 *      { "key": "mybp4", "value": "abstain" },
 *      { "key": "mybp5", "value": "yes" }
 *   ]
 * }
 * ```
 */
struct [[eosio::table("votes"), eosio::contract("volentixwork")]] votes_row {
    name                proposal_name;
    map<name, name>     votes;

    uint64_t primary_key() const { return proposal_name.value; }
};

typedef eosio::multi_index< "votes"_n, votes_row> votes_table;

/**
 * ## TABLE `state`
 *
 * - `{time_point_sec} current_voting_period` - current voting period
 * - `{time_point_sec} next_voting_period` - next voting period
 * - `{asset} [liquid_deposits="0.0000 VTX"]` - liquid deposits
 * - `{asset} [locked_deposits="0.0000 VTX"]` - locked deposits
 * - `{asset} [available_funding="0.0000 VTX"]` - available funding
 *
 * ### example
 *
 * ```json
 * {
 *   "current_voting_period": "2019-12-12T00:00:00",
 *   "next_voting_period": "2020-01-11T00:00:00",
 *   "liquid_deposits": "100.0000 VTX",
 *   "locked_deposits": "200.0000 VTX",
 *   "available_funding": "50000.0000 VTX",
 * }
 * ```
 */
struct [[eosio::table("state"), eosio::contract("volentixwork")]] state_row {
    time_point_sec       current_voting_period;
    time_point_sec       next_voting_period;
    asset                liquid_deposits = asset{0, BUDGET_SYMBOL};
    asset                locked_deposits = asset{0, BUDGET_SYMBOL};
    asset                available_funding = asset{0, BUDGET_SYMBOL};
};

 typedef eosio::singleton< "state"_n, state_row> state_table;

/**
 * ## TABLE `deposits`
 *
 * - `{name} account` - account balance owner
 * - `{asset} balance` - token balance amount
 *
 * ### example
 *
 * ```json
 * {
 *   "account": "myaccount",
 *   "balance": "100.0000 VTX"
 * }
 * ```
 */
struct [[eosio::table("deposits"), eosio::contract("volentixwork")]] deposits_row {
    name         account;
    asset        balance;

    uint64_t primary_key() const { return account.value; }
};

typedef eosio::multi_index< "deposits"_n, deposits_row > deposits_table;

/**
 * ## TABLE `periods`
 *
 * - `{time_point_sec} voting_period` - voting period
 * - `{set<name>} proposals` - set of proposal names
 *
 * ### example
 *
 * ```json
 * {
 *   "voting_period": "2019-11-01T00:00:00",
 *   "proposals": ["mywps"],
 * }
 * ```
 */
struct [[eosio::table("periods"), eosio::contract("volentixwork")]] periods_row {
    time_point_sec      voting_period;
    set<name>           proposals;

    uint64_t primary_key() const { return voting_period.sec_since_epoch(); }
};

typedef eosio::multi_index< "periods"_n, periods_row > periods_table;

/**
 * ## TABLE `claims`
 *
 * - `{uint64_t} id` - claim identifier
 * - `{name} proposer` - proposer
 * - `{name} proposal_name` - proposal name
 * - `{asset} quantity` - claim quantity amount
 * - `{time_point_sec} timestamp` - timestamp of claim transfer
 * - `{checksum256} tx_id` - transaction ID
 *
 * ### example
 *
 * ```json
 * {
 *   "id": 0,
 *   "proposer": "myaccount",
 *   "proposal_name": "mywps",
 *   "quantity": "100.0000 VTX",
 *   "timestamp": "2019-12-01T00:00:00",
 *   "tx_id": "<TRANSACTION ID>"
 * }
 * ```
 */
struct [[eosio::table("claims"), eosio::contract("volentixwork")]] claims_row {
    uint64_t         id;
    name             proposer;
    name             proposal_name;
    asset            quantity;
    time_point_sec   timestamp;
    checksum256      tx_id;

    uint64_t primary_key() const { return id; }
    uint64_t by_proposer() const { return proposer.value; }
    uint64_t by_proposal_name() const { return proposal_name.value; }
};

typedef eosio::multi_index< "claims"_n, claims_row,
    indexed_by<"byproposer"_n, const_mem_fun<claims_row, uint64_t, &claims_row::by_proposer>>,
    indexed_by<"byproposal"_n, const_mem_fun<claims_row, uint64_t, &claims_row::by_proposal_name>>
> claims_table;

private:
    // local instances of the multi indexes
    proposals_table             _proposals;
    votes_table                 _votes;
    settings_table              _settings;
    state_table                 _state;
    deposits_table              _deposits;
    proposers_table             _proposers;
    periods_table               _periods;
    claims_table                _claims;
};

} // eosio namespace

// Help functions
template <typename T>
bool is_in(const T& v, std::initializer_list<uint8_t> lst)
{
    return std::find(std::begin(lst), std::end(lst), v) != std::end(lst);
}

