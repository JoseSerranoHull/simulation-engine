#pragma once

/* parasoft-begin-suppress ALL */
#include <string>
#include <unordered_map>
#include <utility>
/* parasoft-end-suppress ALL */

namespace GE::Physics {

    /**
     * @struct MaterialInteractionRecord
     * @brief Per-pair physical interaction parameters looked up at collision time.
     */
    struct MaterialInteractionRecord {
        float restitution     { 0.6f };
        float staticFriction  { 0.0f };
        float dynamicFriction { 0.0f };
    };

    /**
     * @class MaterialInteractionRegistry
     * @brief Maps canonical (materialA, materialB) pairs to MaterialInteractionRecord.
     * Keys are normalised alphabetically so (A,B) and (B,A) hit the same entry.
     * Populated at scene-load time from the FlatBuffers MaterialInteraction table.
     */
    class MaterialInteractionRegistry {
    public:
        void Register(const std::string& a, const std::string& b,
                      float restitution, float staticFric, float dynFric)
        {
            m_map[MakeKey(a, b)] = { restitution, staticFric, dynFric };
        }

        /** @returns true and fills \p out when a matching pair is found. */
        bool Lookup(const std::string& a, const std::string& b,
                    MaterialInteractionRecord& out) const
        {
            const auto it = m_map.find(MakeKey(a, b));
            if (it != m_map.end()) {
                out = it->second;
                return true;
            }
            return false;
        }

        void Clear() { m_map.clear(); }

    private:
        struct PairHash {
            std::size_t operator()(const std::pair<std::string, std::string>& p) const noexcept {
                const std::size_t h1 = std::hash<std::string>{}(p.first);
                const std::size_t h2 = std::hash<std::string>{}(p.second);
                return h1 ^ (h2 << 1);
            }
        };

        static std::pair<std::string, std::string>
        MakeKey(const std::string& a, const std::string& b)
        {
            return (a <= b) ? std::make_pair(a, b) : std::make_pair(b, a);
        }

        std::unordered_map<std::pair<std::string, std::string>,
                           MaterialInteractionRecord,
                           PairHash> m_map;
    };

} // namespace GE::Physics
