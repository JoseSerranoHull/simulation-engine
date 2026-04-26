// Winsock2 must come before windows.h
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
/* parasoft-begin-suppress ALL */
#include <winsock2.h>
#include <ws2tcpip.h>
/* parasoft-end-suppress ALL */
#pragma comment(lib, "Ws2_32.lib")

#include "networking/NetworkService.h"
#include "core/Logger.h"

/* parasoft-begin-suppress ALL */
#include <cstring>
#include <array>
/* parasoft-end-suppress ALL */

// Compile-time guard: uintptr_t must safely hold a SOCKET value.
static_assert(sizeof(uintptr_t) >= sizeof(SOCKET),
    "uintptr_t is too small to hold a SOCKET on this platform");

namespace GE::Networking {

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

bool NetworkService::Init(uint16_t localPort) {
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        GE_LOG_ERROR("NetworkService: WSAStartup failed (" +
                     std::to_string(WSAGetLastError()) + ")");
        return false;
    }

    const SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        GE_LOG_ERROR("NetworkService: socket() failed (" +
                     std::to_string(WSAGetLastError()) + ")");
        WSACleanup();
        return false;
    }

    // Bind to all interfaces on the requested port
    sockaddr_in local{};
    local.sin_family      = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port        = htons(localPort);

    if (bind(sock, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == SOCKET_ERROR) {
        GE_LOG_ERROR("NetworkService: bind() failed on port " +
                     std::to_string(localPort) + " (" +
                     std::to_string(WSAGetLastError()) + ")");
        closesocket(sock);
        WSACleanup();
        return false;
    }

    // Set non-blocking mode
    u_long mode = 1;
    if (ioctlsocket(sock, FIONBIO, &mode) == SOCKET_ERROR) {
        GE_LOG_ERROR("NetworkService: ioctlsocket(FIONBIO) failed (" +
                     std::to_string(WSAGetLastError()) + ")");
        closesocket(sock);
        WSACleanup();
        return false;
    }

    m_socket      = static_cast<uintptr_t>(sock);
    m_initialised = true;
    GE_LOG_INFO("NetworkService: listening on UDP port " + std::to_string(localPort));
    return true;
}

void NetworkService::Shutdown() {
    if (m_initialised) {
        closesocket(static_cast<SOCKET>(m_socket));
        m_socket      = INVALID_SOCK;
        m_initialised = false;
        WSACleanup();
        GE_LOG_INFO("NetworkService: shut down.");
    }
}

// ---------------------------------------------------------------------------
// Peer management
// ---------------------------------------------------------------------------

void NetworkService::AddPeer(uint8_t peerId, const std::string& ip, uint16_t port) {
    if (peerId < 1 || peerId > MAX_PEERS) {
        GE_LOG_ERROR("NetworkService: AddPeer invalid peerId " + std::to_string(peerId));
        return;
    }

    const uint8_t idx = peerId - 1U;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        GE_LOG_ERROR("NetworkService: AddPeer invalid IP '" + ip + "'");
        return;
    }

    m_peers[idx].active = true;
    m_peers[idx].addr   = addr.sin_addr.s_addr;
    m_peers[idx].port   = addr.sin_port;

    GE_LOG_INFO("NetworkService: peer " + std::to_string(peerId) +
                " = " + ip + ":" + std::to_string(port));
}

// ---------------------------------------------------------------------------
// Transmission
// ---------------------------------------------------------------------------

void NetworkService::Send(uint8_t peerId, const void* data, std::size_t size) {
    if (!m_initialised || peerId < 1 || peerId > MAX_PEERS) { return; }

    const PeerEntry& p = m_peers[peerId - 1U];
    if (!p.active) { return; }

    sockaddr_in dest{};
    dest.sin_family      = AF_INET;
    dest.sin_addr.s_addr = p.addr;
    dest.sin_port        = p.port;

    sendto(static_cast<SOCKET>(m_socket),
           reinterpret_cast<const char*>(data),
           static_cast<int>(size), 0,
           reinterpret_cast<sockaddr*>(&dest),
           static_cast<int>(sizeof(dest)));
}

void NetworkService::Broadcast(const void* data, std::size_t size) {
    for (uint8_t i = 1; i <= MAX_PEERS; ++i) {
        Send(i, data, size);
    }
}

// ---------------------------------------------------------------------------
// Reception
// ---------------------------------------------------------------------------

void NetworkService::Poll(const ReceiveCallback& cb) {
    if (!m_initialised || !cb) { return; }

    static constexpr int BUFSIZE = 2048;
    static char buf[BUFSIZE];

    sockaddr_in from{};
    int fromLen = static_cast<int>(sizeof(from));

    for (;;) {
        const int received = recvfrom(
            static_cast<SOCKET>(m_socket),
            buf, BUFSIZE, 0,
            reinterpret_cast<sockaddr*>(&from),
            &fromLen);

        if (received == SOCKET_ERROR) {
            const int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                break; // No more data available right now — normal exit
            }
            if (err == WSAECONNRESET) {
                // Windows UDP quirk: ICMP "port unreachable" from a peer with
                // no listener.  Not fatal — the remote peer simply isn't running
                // yet.  Continue draining the error queue.
                continue;
            }
            GE_LOG_ERROR("NetworkService: recvfrom error (" + std::to_string(err) + ")");
            break;
        }

        if (received < static_cast<int>(sizeof(Packets::Header))) {
            continue; // Packet too small to contain a header — discard
        }

        Packets::Header hdr{};
        std::memcpy(&hdr, buf, sizeof(hdr));

        cb(hdr.senderId,
           reinterpret_cast<const uint8_t*>(buf),
           static_cast<std::size_t>(received),
           from.sin_addr.s_addr,
           from.sin_port);
    }
}

// ---------------------------------------------------------------------------
// Discovery helpers
// ---------------------------------------------------------------------------

void NetworkService::SendRaw(uint32_t addr, uint16_t port,
                              const void* data, std::size_t size) {
    if (!m_initialised) { return; }
    sockaddr_in dest{};
    dest.sin_family      = AF_INET;
    dest.sin_addr.s_addr = addr;   // already network byte order
    dest.sin_port        = port;   // already network byte order
    sendto(static_cast<SOCKET>(m_socket),
           reinterpret_cast<const char*>(data),
           static_cast<int>(size), 0,
           reinterpret_cast<sockaddr*>(&dest),
           static_cast<int>(sizeof(dest)));
}

bool NetworkService::EnableBroadcast() {
    if (!m_initialised) { return false; }
    const int yes = 1;
    const bool ok = setsockopt(static_cast<SOCKET>(m_socket),
                               SOL_SOCKET, SO_BROADCAST,
                               reinterpret_cast<const char*>(&yes),
                               static_cast<int>(sizeof(yes))) == 0;
    if (!ok) {
        GE_LOG_ERROR("NetworkService: EnableBroadcast failed (" +
                     std::to_string(WSAGetLastError()) + ")");
    }
    return ok;
}

std::string NetworkService::GetLocalIPString() const {
    char hostname[256]{};
    if (gethostname(hostname, sizeof(hostname)) != 0) { return "?.?.?.?"; }

    addrinfo hints{};
    addrinfo* res = nullptr;
    hints.ai_family = AF_INET;
    if (getaddrinfo(hostname, nullptr, &hints, &res) != 0 || res == nullptr) {
        return "?.?.?.?";
    }

    char buf[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET,
              &reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr,
              buf, sizeof(buf));
    freeaddrinfo(res);
    return buf;
}

} // namespace GE::Networking
