#pragma once
#ifdef _WIN32
#include <winsock2.h>

extern "C" void RefuseClosedLoopbackAtOnce_nid_no_patch(SOCKET socket, const sockaddr* address);
#elif defined(__APPLE__)
#include <sys/socket.h>

extern "C" int ConnectRetryingLoopbackReset_nid_no_patch(int descriptor, const sockaddr* address, socklen_t size, bool blockingStream);
#endif
