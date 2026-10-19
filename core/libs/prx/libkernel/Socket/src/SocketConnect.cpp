#ifdef _WIN32
#include "prx/libkernel/Socket/include/SocketConnect.hpp"
#include <ws2tcpip.h>
#include <mstcpip.h>
#include <iphlpapi.h>
#include <vector>

namespace {

bool IsLoopback(const sockaddr* address) {
    if (address->sa_family == AF_INET) return (ntohl(reinterpret_cast<const sockaddr_in*>(address)->sin_addr.s_addr) >> 24) == 127;
    if (address->sa_family == AF_INET6) return IN6_IS_ADDR_LOOPBACK(&reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr);
    return false;
}

u_short Port(const sockaddr* address) {
    return address->sa_family == AF_INET ? reinterpret_cast<const sockaddr_in*>(address)->sin_port : reinterpret_cast<const sockaddr_in6*>(address)->sin6_port;
}

template<typename TTable>
bool ListenerOnPort(ULONG family, u_short port) {
    std::vector<unsigned char> table;
    ULONG size = 0;
    DWORD result = ERROR_INSUFFICIENT_BUFFER;
    while (result == ERROR_INSUFFICIENT_BUFFER) {
        table.resize(size);
        result = GetExtendedTcpTable(table.data(), &size, FALSE, family, TCP_TABLE_OWNER_PID_LISTENER, 0);
    }
    if (result != NO_ERROR) return true;
    const auto* rows = reinterpret_cast<const TTable*>(table.data());
    for (DWORD i = 0; i < rows->dwNumEntries; ++i) {
        if (static_cast<u_short>(rows->table[i].dwLocalPort) == port) return true;
    }
    return false;
}

}

extern "C" void RefuseClosedLoopbackAtOnce_nid_no_patch(SOCKET socket, const sockaddr* address) {
    if (!IsLoopback(address)) return;
    const u_short port = Port(address);
    if (ListenerOnPort<MIB_TCPTABLE_OWNER_PID>(AF_INET, port) || ListenerOnPort<MIB_TCP6TABLE_OWNER_PID>(AF_INET6, port)) return;
    TCP_INITIAL_RTO_PARAMETERS parameters{TCP_INITIAL_RTO_UNSPECIFIED_RTT, TCP_INITIAL_RTO_NO_SYN_RETRANSMISSIONS};
    DWORD returned = 0;
    WSAIoctl(socket, SIO_TCP_INITIAL_RTO, &parameters, sizeof(parameters), nullptr, 0, &returned, nullptr, nullptr);
}
#elif defined(__APPLE__)
#include "prx/libkernel/Socket/include/SocketConnect.hpp"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <fcntl.h>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <thread>

namespace {

bool IsLoopback(const sockaddr* address) {
    if (address->sa_family == AF_INET) return (ntohl(reinterpret_cast<const sockaddr_in*>(address)->sin_addr.s_addr) >> 24) == 127;
    if (address->sa_family == AF_INET6) return IN6_IS_ADDR_LOOPBACK(&reinterpret_cast<const sockaddr_in6*>(address)->sin6_addr);
    return false;
}

bool ReplaceResetSocket(int descriptor, int domain) {
    const int fresh = ::socket(domain, SOCK_STREAM, 0);
    if (fresh < 0) return false;
    static constexpr int Options[][2] = {{SOL_SOCKET, SO_REUSEADDR}, {SOL_SOCKET, SO_REUSEPORT}, {SOL_SOCKET, SO_KEEPALIVE}, {SOL_SOCKET, SO_OOBINLINE},
        {SOL_SOCKET, SO_SNDBUF}, {SOL_SOCKET, SO_RCVBUF}, {SOL_SOCKET, SO_SNDLOWAT}, {SOL_SOCKET, SO_RCVLOWAT}, {SOL_SOCKET, SO_NOSIGPIPE}, {IPPROTO_TCP, TCP_NODELAY}};
    for (const auto& option : Options) {
        int value = 0;
        socklen_t size = sizeof(value);
        if (::getsockopt(descriptor, option[0], option[1], &value, &size) == 0) ::setsockopt(fresh, option[0], option[1], &value, size);
    }
    linger lingering{};
    socklen_t lingerSize = sizeof(lingering);
    if (::getsockopt(descriptor, SOL_SOCKET, SO_LINGER, &lingering, &lingerSize) == 0) ::setsockopt(fresh, SOL_SOCKET, SO_LINGER, &lingering, lingerSize);
    const int statusFlags = ::fcntl(descriptor, F_GETFL);
    const int descriptorFlags = ::fcntl(descriptor, F_GETFD);
    const bool replaced = ::dup2(fresh, descriptor) == descriptor;
    ::close(fresh);
    if (!replaced) return false;
    if (statusFlags >= 0) ::fcntl(descriptor, F_SETFL, statusFlags);
    if (descriptorFlags >= 0) ::fcntl(descriptor, F_SETFD, descriptorFlags);
    return true;
}

}

extern "C" int ConnectRetryingLoopbackReset_nid_no_patch(int descriptor, const sockaddr* address, socklen_t size, bool blockingStream) {
    if (::connect(descriptor, address, size) == 0) return 0;
    if (errno != ECONNRESET || !blockingStream || !IsLoopback(address)) return -1;
    auto wait = std::chrono::milliseconds(25);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(75);
    while (std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(wait);
        wait = std::min(wait * 2, std::chrono::milliseconds(1000));
        if (!ReplaceResetSocket(descriptor, address->sa_family)) {
            errno = ECONNRESET;
            return -1;
        }
        if (::connect(descriptor, address, size) == 0) return 0;
        if (errno != ECONNRESET) return -1;
    }
    return -1;
}
#endif
