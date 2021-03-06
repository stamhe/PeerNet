#include "PeerNet.h"
#pragma comment(lib, "ws2_32.lib")

#include <unordered_map>
#include <deque>
#include <mutex>

using std::string;
using std::to_string;
using std::deque;
using std::unordered_map;
using std::mutex;

namespace PeerNet
{
	namespace
	{
		//	Main Variables
		bool Buffers_Init = false;
		RIO_EXTENSION_FUNCTION_TABLE g_rio;

		AddressPool<NetPeer*, MaxPeers>* PeerKeeper = nullptr;
		AddressPool<NetSocket*, MaxSockets>* SocketKeeper = nullptr;

		//	LoopBack Socket
		NetSocket* Sock_LoopBack = nullptr;
		//	LocalHost Peer
		NetPeer* Peer_LocalHost = nullptr;
	}

	NetSocket*const OpenSocket(string StrIP, string StrPort)
	{
		NetSocket* ExistingSocket = NULL;
		NetAddress* NewAddress;
		//	Can we create a new NetSocket with this ip/port?
		if (SocketKeeper->New(StrIP, StrPort, ExistingSocket, NewAddress))
		{
			NetSocket* ThisSocket = new NetSocket(NewAddress);
			SocketKeeper->InsertConnected(NewAddress, ThisSocket);
			return ThisSocket;
		}
		//	No available connections or object already connected
		return ExistingSocket;
	}

	//	ToDo: The NetAddress created through NewAddress needs to be returned to PeerKeeper's Unused container
	//	when this NetPeer is cleaned up
	NetPeer*const ConnectPeer(string StrIP, string StrPort, NetSocket* DefaultSocket)
	{
		if (DefaultSocket == nullptr) { printf("Error: DefaultSocket NULL\n"); return nullptr; }
		NetPeer* ExistingPeer = nullptr;
		NetAddress* NewAddress = nullptr;
		//	Can we create a new NetPeer with this ip/port?
		if (PeerKeeper->New(StrIP, StrPort, ExistingPeer, NewAddress))
		{
			NetPeer* ThisPeer = new NetPeer(DefaultSocket, NewAddress);
			PeerKeeper->InsertConnected(NewAddress, ThisPeer);
			return ThisPeer;
		}
		//	No available connections or object already connected
		return ExistingPeer;
	}

	//	Need DisconnectPeer/CloseSocket to properly cleanup our internal containers
	//	Or split those functions up into their respective files
	//	And let their respective classes destructors handle it <--
	void DisconnectPeer(SOCKADDR_INET* AddrBuff)
	{

	}

	NetPeer*const GetPeer(SOCKADDR_INET* AddrBuff, NetSocket* DefaultSocket)
	{
		//	See if we already have an existing peer
		NetPeer* ThisPeer = PeerKeeper->GetExisting(AddrBuff);
		if (ThisPeer != nullptr) { return ThisPeer; }

		const string SenderIP(inet_ntoa(AddrBuff->Ipv4.sin_addr));
		const string SenderPort(to_string(ntohs(AddrBuff->Ipv4.sin_port)));

		return ConnectPeer(SenderIP, SenderPort, DefaultSocket);
	}

	//	Returns access to the RIO Function Table
	RIO_EXTENSION_FUNCTION_TABLE RIO() { return g_rio; }

	// Public Implementation Methods
	void Initialize()
	{
		printf("Initializing PeerNet\n");
		//	Startup WinSock 2.2
		const size_t iResult = WSAStartup(MAKEWORD(2, 2), &WSADATA());
		if (iResult != 0) {
			printf("\tWSAStartup Error: %i\n", (int)iResult);
		} else {
			//	Create a dummy socket long enough to get our RIO Function Table pointer
			SOCKET RioSocket = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, NULL, WSA_FLAG_REGISTERED_IO);
			GUID functionTableID = WSAID_MULTIPLE_RIO;
			DWORD dwBytes = 0;
			if (WSAIoctl(RioSocket, SIO_GET_MULTIPLE_EXTENSION_FUNCTION_POINTER,
				&functionTableID,
				sizeof(GUID),
				(void**)&g_rio,
				sizeof(g_rio),
				&dwBytes, 0, 0) == SOCKET_ERROR) {
				printf("RIO Failed(%i)\n", WSAGetLastError());
			}
			closesocket(RioSocket);

			//	Create the Address Pool handlers
			PeerKeeper = new AddressPool<NetPeer*, MaxPeers>();
			SocketKeeper = new AddressPool<NetSocket*, MaxSockets>();

			//	Initialize Peer Address Memory Buffer
			printf("\tPeer Buffer: ");
			PeerKeeper->Addr_BufferID = g_rio.RIORegisterBuffer(PeerKeeper->Addr_Buffer, sizeof(SOCKADDR_INET)*MaxPeers);
			if (PeerKeeper->Addr_BufferID == RIO_INVALID_BUFFERID) { printf("Peer Address Memory Buffer: Invalid BufferID\n"); }
			for (DWORD i = 0, AddressOffset = 0; i < MaxPeers; i++/*, AddressOffset += sizeof(SOCKADDR_INET)*/)
			{
				NetAddress* PeerAddress = new NetAddress();
				PeerAddress->BufferId = PeerKeeper->Addr_BufferID;
				PeerAddress->Offset = AddressOffset;
				PeerAddress->Length = sizeof(SOCKADDR_INET);
				PeerKeeper->UnusedAddr.push_front(PeerAddress);

				AddressOffset += sizeof(SOCKADDR_INET);
			}
			printf("%I64u\n", PeerKeeper->UnusedAddr.size());
			//	Initialize Socket Address Memory Buffer
			printf("\tSocket Buffer: ");
			SocketKeeper->Addr_BufferID = g_rio.RIORegisterBuffer(SocketKeeper->Addr_Buffer, sizeof(SOCKADDR_INET)*MaxSockets);
			if (SocketKeeper->Addr_BufferID == RIO_INVALID_BUFFERID) { printf("Socket Address Memory Buffer: Invalid BufferID\n"); }
			for (DWORD i = 0, AddressOffset = 0; i < MaxSockets; i++/*, AddressOffset += sizeof(SOCKADDR_INET)*/)
			{
				NetAddress* SocketAddress = new NetAddress();
				SocketAddress->BufferId = SocketKeeper->Addr_BufferID;
				SocketAddress->Offset = AddressOffset;
				SocketAddress->Length = sizeof(SOCKADDR_INET);
				SocketKeeper->UnusedAddr.push_front(SocketAddress);

				AddressOffset += sizeof(SOCKADDR_INET);
			}
			printf("%I64u\n", SocketKeeper->UnusedAddr.size());
			Sock_LoopBack = OpenSocket("127.0.0.1", to_string(PN_LoopBackPort));
			Peer_LocalHost = ConnectPeer("127.0.0.1", to_string(PN_LoopBackPort), Sock_LoopBack);
			printf("Initialization Complete\n");
		}
	}

	NetSocket*const LoopBack() { return Sock_LoopBack; }
	NetPeer*const LocalHost() { return Peer_LocalHost; }

	void Deinitialize()
	{
		printf("Deinitializing PeerNet\n");

		delete Peer_LocalHost;
		delete Sock_LoopBack;

		WSACleanup();
		delete PeerKeeper;
		delete SocketKeeper;
		printf("Deinitialization Complete\n");
	}
}