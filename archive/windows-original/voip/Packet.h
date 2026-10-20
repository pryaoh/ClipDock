#ifndef  __Sync_Packet_H
#define  __Sync_Packet_H

#include "Object.h"
#include "String.h"
#include "global.h"

#define		PACKETBUFFERSIZE	USHRT_MAX
#define		PACKETHEADERSIZE	4

class InvalidPacketException : public Object
{
};

class Packet : public Object
{
public:
			Packet();
			Packet( unsigned short idValue );
			Packet( const Packet& source );
	virtual ~Packet();

			bool			isValidHeader();
			bool			isValidPacket();

			Packet&			id( unsigned short ID );
			unsigned short	id();
			unsigned short	getDataFieldSize();
			int				getPacketSize(){ return ( getDataFieldSize() + PACKETHEADERSIZE ); }
			int				getReceivedSize(){ return receivedSize; }

			void			clear( int bufferSize = PACKETBUFFERSIZE );
			void			increaseBuffer(int increaseSize);

			char*			getPacketBuffer(){ return packetBuffer; }
			void			copyToBuffer( char* buff, int size );

			Packet&	operator = ( Packet& packet );

			Packet&	operator << ( bool arg );
			Packet&	operator << ( int arg );
			Packet&	operator << ( long arg );
			Packet&	operator << ( DWORD arg );
			Packet& operator << ( __int64 arg );
			Packet&	operator << ( LPTSTR arg );
			Packet&	operator << ( String arg );
			Packet& operator << ( Packet& arg );

			Packet&	operator >> ( bool& arg );
			Packet&	operator >> ( int& arg );
			Packet&	operator >> ( long& arg );
			Packet&	operator >> ( DWORD& arg );
			Packet& operator >> ( __int64& arg );
			Packet& operator >> ( LPTSTR arg );
			Packet&	operator >> ( String& arg );
			Packet& operator >> ( Packet& arg );

			void		readData( void* buffer, int size ) ;
			void		readData2( void* buffer) ;
			void		writeData( void* buffer, int size ) ;
			void		writeData2( void* buffer,unsigned short size) ;
protected:
			unsigned long	PacketBufferSize;
	typedef struct
	{
		unsigned short*		dataSize;
		unsigned short*		protocolID;
	} HEADER;

			HEADER		packetHeader;
			char		packetBuffer[PACKETBUFFERSIZE];
			char*		dataField;
			char*		readPosition, *writePosition;
			char*		endOfDataField;
			int			receivedSize;
};

#endif