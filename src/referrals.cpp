// Copyright (c) 2017-2018 The Merit Foundation developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "referrals.h"

#include <utility>

namespace referral
{
ReferralsViewCache::ReferralsViewCache(ReferralsViewDB* db) : m_db{db}
{
    assert(db);
};

MaybeReferral ReferralsViewCache::GetReferral(const Address& address) const
{
    {
        LOCK(m_cs_cache);
        auto it = referrals_index.find(address);
        if (it != referrals_index.end()) {
            return *it;
        }
    }

    if (auto ref = m_db->GetReferral(address)) {
        InsertReferralIntoCache(*ref);
        return ref;
    }

    return {};
}

bool ReferralsViewCache::Exists(const uint256& hash) const
{
    {
        LOCK(m_cs_cache);
        if (referrals_index.get<by_hash>().count(hash) > 0) {
            return true;
        }
    }

    if (auto ref = m_db->GetReferral(hash)) {
        InsertReferralIntoCache(*ref);
        return true;
    }

    return false;
}

bool ReferralsViewCache::Exists(const Address& address) const
{
    {
        LOCK(m_cs_cache);
        if (referrals_index.count(address) > 0) {
            return true;
        }
    }
    if (auto ref = m_db->GetReferral(address)) {
        InsertReferralIntoCache(*ref);
        return true;
    }
    return false;
}

bool ReferralsViewCache::Exists(
        const std::string& alias,
        bool normalize_alias) const
{
    if (alias.size() == 0) {
        return false;
    }

    auto maybe_normalized = alias;
    if(normalize_alias) {
        NormalizeAlias(maybe_normalized);
    }

    {
        LOCK(m_cs_cache);
        if (referrals_index.get<by_alias>().count(maybe_normalized) > 0) {
            return true;
        }
    }

    if (auto ref = m_db->GetReferral(maybe_normalized, normalize_alias)) {
        InsertReferralIntoCache(*ref);
        return true;
    }
    return false;
}

void ReferralsViewCache::InsertReferralIntoCache(const Referral& ref) const
{
    LOCK(m_cs_cache);

    if (
        !(ref.alias.size() == 0 ||
        referrals_index.get<by_alias>().count(ref.alias) == 0 ||
        !IsConfirmed(ref.alias, false))
    ) {
        printf("\talias is already in the cache or confirmed: %s", ref.alias.c_str());
    }
    // check that referral alias is not in cache or it's unconfirmed
    assert(
        ref.alias.size() == 0 ||
        referrals_index.get<by_alias>().count(ref.alias) == 0 ||
        !IsConfirmed(ref.alias, false)
        );

    referrals_index.insert(ref);
}

bool ReferralsViewCache::RemoveReferral(const Referral& ref) const
{
    referrals_index.erase(ref.GetAddress());
    return m_db->RemoveReferral(ref);
}

bool ReferralsViewCache::UpdateConfirmation(char address_type, const Address& address, CAmount amount)
{
    assert(m_db);
    CAmount updated_amount;
    //TODO: Have an in memory cache. For now just passthrough.
    auto success = m_db->UpdateConfirmation(address_type, address, amount, updated_amount);

    if (!success) {
        return false;
    }

    // if referral was unconfirmed, remove it from the cache
    if (updated_amount == 0) {
        referrals_index.erase(address);
    }

    return true;
}


bool ReferralsViewCache::IsConfirmed(const Address& address) const
{
    assert(m_db);
    //TODO: Have an in memory cache. For now just passthrough.
    return m_db->IsConfirmed(address);
}

bool ReferralsViewCache::IsConfirmed(const std::string& alias, bool normalize_alias) const
{
    assert(m_db);
    //TODO: Have an in memory cache. For now just passthrough.
    return m_db->IsConfirmed(alias, normalize_alias);
}

}
