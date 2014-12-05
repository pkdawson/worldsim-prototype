#pragma once

#include <vector>
using namespace std;

template<typename TKey, typename TVal>
class CompactMap
{
public:
    CompactMap()
    {
    }

    CompactMap(CompactMap&& src)
    {
        _vec = std::move(src._vec);
    }

    CompactMap& operator=(CompactMap &&src)
    {
        _vec = std::move(src._vec);
        return *this;
    }

    void add(const TKey& k, const TVal& v)
    {
        _vec.emplace_back(k, v);
    }

    bool has(const TKey& key) const
    {
        for (const auto& p : _vec)
        {
            if (p.first == key)
                return true;
        }
        return false;
    }

    TVal& get(const TKey& key)
    {
        for (auto& p : _vec)
        {
            if (p.first == key)
                return p.second;
        }
        throw std::exception();
    }

private:
    vector<pair<TKey, TVal>> _vec;
};
