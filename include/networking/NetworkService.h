#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
#include <string>
#include <functional>
#include <array>
#include <vector>
/* parasoft-end-suppress ALL */

#include "core/Common.h"
#include "networking/Packets.h"

// Forward-declare Winsock2 types so callers never need to include winsock2.h
struct sockaddr_in;

namespace GE::Networking {

    /// Callback: (senderId, rawBytes, byteCount, senderAddr, senderPort)
    /// senderAddr and senderPort are in network byte order (from recvfrom).
    using ReceiveCallback = std::function<void(uint8_t senderId, const uint8_t* data,
                                               std::size_t size,
                                               uint32_t senderAddr, uint16_t senderPort)>;

    /**
     * @class NetworkService
     * @brief Non-blocking UDP service (Winsock2). All platform types are hidden.
     *
     * Isolation rule: this header must never include ECS, Vulkan, or scene headers.
     * Callers see only cstdint / string / functional / Packets.h.
     */
    class NetworkService {
    public:
        static constexpr uint8_t MAX_PEERS = 4;

        NetworkService()  = default;
        ~NetworkService() = default;

        // Non-copyable — owns a socket handle
        NetworkService(const NetworkService&)            = delete;
        NetworkService& operator=(const NetworkService&) = delete;

        // --- Lifecycle ---

        /** @brief Starts Winsock, creates and binds a non-blocking UDP socket. */
        bool Init(uint16_t localPort);

        /** @brief Closes the socket and calls WSACleanup. */
        void Shutdown();

        // --- Peer Management ---

        /**
         * @brief Registers a remote peer.
         * @param peerId  1-based peer index (1–4).
         * @param ip      IPv4 address string (e.g. "127.0.0.1").
         * @param port    UDP port the peer is listening on.
         */
        void AddPeer(uint8_t peerId, const std::string& ip, uint16_t port);

        /** @brief Removes a single peer entry so its slot becomes available for reconnect. */
        void RemovePeer(uint8_t peerId);

        // --- Transmission ---

        /** @brief Sends raw bytes to a specific peer (1-based ID). */
        void Send(uint8_t peerId, const void* data, std::size_t size);

        /** @brief Sends raw bytes to all registered peers. */
        void Broadcast(const void* data, std::size_t size);

        // --- Reception ---

        /**
         * @brief Drains the receive queue (non-blocking).
         * Calls cb for every datagram received since the last Poll().
         * Returns immediately when no data is waiting.
         */
        void Poll(const ReceiveCallback& cb);

        // --- Discovery helpers ---

        /** @brief One-off unicast to an arbitrary address (used for DiscoveryResponse). */
        void SendRaw(uint32_t addr, uint16_t port, const void* data, std::size_t size);

        /** @brief Enables SO_BROADCAST on the bound socket. Call once after Init(). */
        bool EnableBroadcast();

        /** @brief Returns the local IPv4 address as a dotted-decimal string. */
        std::string GetLocalIPString() const;

        // --- State Queries ---

        bool    IsConnected()    const { return m_initialised; }
        uint8_t GetLocalPeerId() const { return m_localPeerId; }
        void    SetLocalPeerId(uint8_t id) { m_localPeerId = id; }

        /** @brief Returns true if peerId (1–4) is currently registered and active. */
        bool HasPeer(uint8_t peerId) const {
            if (peerId < 1U || peerId > MAX_PEERS) { return false; }
            return m_peers[static_cast<std::size_t>(peerId - 1U)].active;
        }

        /** @brief Snapshot of active peers — used by NetworkBridge to relay peer lists. */
        struct ActivePeer {
            uint8_t  peerId { 0 };
            uint32_t addr   { 0 };  ///< NBO
            uint16_t port   { 0 };  ///< NBO
        };
        std::vector<ActivePeer> GetActivePeers() const;

    private:
        // We store the socket as uintptr_t to avoid exposing <winsock2.h> in this header.
        // INVALID_SOCKET == ~0ULL on 64-bit; we initialise to that sentinel.
        static constexpr uintptr_t INVALID_SOCK = ~static_cast<uintptr_t>(0);

        uintptr_t m_socket     { INVALID_SOCK };
        bool      m_initialised{ false };
        uint8_t   m_localPeerId{ 1 };

        struct PeerEntry {
            bool     active { false };
            uint32_t addr   { 0 };   ///< sin_addr.s_addr (network byte order)
            uint16_t port   { 0 };   ///< sin_port       (network byte order)
        };

        // Indexed by (peerId - 1), i.e. peers 1–4 → indices 0–3
        std::array<PeerEntry, MAX_PEERS> m_peers {};
    };

} // namespace GE::Networking
