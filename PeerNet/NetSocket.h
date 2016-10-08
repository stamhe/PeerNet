#pragma once


namespace PeerNet
{

	struct RIO_BUF_EXT : public RIO_BUF
	{
		PRIO_BUF pAddrBuff;

		//	Values Filled Upon Call To GetQueuedCompletionStatus
		//	Unused and basically just a way to allocate these variables upfront and only once
		DWORD numberOfBytes = 0;
		ULONG_PTR completionKey = 0;
		//
	}; typedef RIO_BUF_EXT* PRIO_BUF_EXT;

	class NetSocket
	{
		std::deque<PRIO_BUF_EXT> AddrBuffs;
		std::deque<PRIO_BUF_EXT> SendBuffs;


		RIO_EXTENSION_FUNCTION_TABLE g_rio;
		SOCKET Socket;
		std::string FormattedAddress;
		const DWORD PendingRecvs;
		const DWORD PendingSends;
		const DWORD PacketSize;
		const DWORD AddrSize;
		PCHAR const p_addr_dBuffer;
		PCHAR const p_recv_dBuffer;
		PCHAR const p_send_dBuffer;
		const HANDLE g_recv_IOCP;
		const HANDLE g_send_IOCP;
		OVERLAPPED* recv_overlapped;
		OVERLAPPED* send_overlapped;
		RIO_CQ g_recv_cQueue;
		RIO_CQ g_send_cQueue;
		RIO_RQ g_requestQueue;

		RIORESULT g_recv_Results[128];
		RIORESULT g_send_Results[128];


		char*const uncompressed_data;

		std::map<unsigned long, NetPacket*const> q_OutgoingUnreliable;
		std::map<unsigned long, NetPacket*const> q_OutgoingOrdered;
		std::map<unsigned long, NetPacket*const> q_OutgoingReliable;
		std::mutex OutgoingMutex;

		std::condition_variable OutgoingCondition;

		bool Initialized;
		void OutgoingFunction();
		void IncomingFunction();
		std::thread OutgoingThread;
		std::thread IncomingThread;

		void NetSocket::CompressAndSendPacket(PRIO_BUF_EXT pBuffer, const NetPacket* const Packet);

	public:
		NetSocket(const std::string StrIP, const std::string StrPort);
		~NetSocket();

		std::shared_ptr<NetPeer> DiscoverPeer(const std::string StrIP, const std::string StrPort);

		void AddOutgoingPacket(NetPacket*const Packet);
		const std::string GetFormattedAddress() const;
	};

	std::shared_ptr<NetPeer> RetrievePeer(const std::string FormattedAddress, NetSocket*const Socket);
}