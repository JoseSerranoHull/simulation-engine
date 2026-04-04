#pragma once

/* parasoft-begin-suppress ALL */
#include <cstdint>
#include <string>
#include <functional>
#include <array>
/* parasoft-end-suppress ALL */

#include "networking/Packets.h"

// Forward-declare Winsock2 types so callers never need to include winsock2.h
struct sockaddr_in;

namespace GE::Networking {

    /// Callback signature: (senderId, rawBytes, byteCount)
    using ReceiveCallback = std::function<void(uint8_t senderId, const uint8_t* data, std::size_t size)>;

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

        // --- State Queries ---

        bool    IsConnected()    const { return m_initialised; }
        uint8_t GetLocalPeerId() const { return m_localPeerId; }
        void    SetLocalPeerId(uint8_t id) { m_localPeerId = id; }

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
