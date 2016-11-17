#pragma once
#include "TimedEvent.hpp"
#include "Channel.hpp"

namespace PeerNet
{
	class NetPeer : public TimedEvent
	{
		NetAddress*const Address;

		KeepAliveChannel* CH_KOL;
		OrderedChannel* CH_Ordered;
		ReliableChannel* CH_Reliable;
		UnreliableChannel* CH_Unreliable;

		void OnTick();
		void OnExpire();

	public:
		NetSocket*const Socket;

		NetPeer(NetSocket*const DefaultSocket, NetAddress*const NetAddr);

		//NetPeer(const std::string StrIP, const std::string StrPort, NetSocket*const DefaultSocket);
		~NetPeer();

		//	Construct and return a NetPacket to fill and send to this NetPeer
		auto NetPeer::CreateNewPacket(const PacketType pType) {
			if (pType == PN_KeepAlive)
			{
				return CH_KOL->NewPacket();
			}
			else if (pType == PN_Ordered)
			{
				return CH_Ordered->NewPacket();
			}
			else if (pType == PN_Reliable)
			{
				return CH_Reliable->NewPacket();
			}

			return CH_Unreliable->NewPacket();
		}

		void Receive_Packet(u_short TypeID, const PCHAR IncomingData, const size_t DataSize, const size_t MaxDataSize, char*const CompressionBuffer);
		void Send_Packet(SendPacket* Packet);

		const size_t CompressPacket(SendPacket * const OUT_Packet, PCHAR DataBuffer, const size_t MaxDataSize);

		const auto RTT_KOL() const { return CH_KOL->RTT(); }

		NetAddress*const GetAddress() const { return Address; }
		//const auto FormattedAddress() const { return Address->FormattedAddress(); }
		//const auto SockAddr() const { return Address->SockAddr(); }
	};
}