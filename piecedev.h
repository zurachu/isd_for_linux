#ifndef __piecedev_h_
#define __piecedev_h_

#include <libusb.h>
#include <stdint.h>
#include <cstdio>

namespace n {
namespace piece {

class UsbDevHandle {
	libusb_device_handle *usb_dev_;

public:
	UsbDevHandle();
	~UsbDevHandle();
	operator libusb_device_handle *() const { return usb_dev_; }
};

class Device {
	UsbDevHandle usb_dev_;

	uint32_t hard_ver_,bios_ver_,bios_date_;
	uint32_t sys_clock_,vdde_voltage_;
	uint32_t sram_top_,sram_end_;
	uint32_t pffs_top_,pffs_end_;
	uint32_t &mblk_adr_;

	enum COMMAND {
		GET_VERSION=0,
		READ_MEM=2,
		WRITE_MEM=3,
		SET_APPSTAT=4,
		GET_APPSTAT=5,
		ERASE_FLASH=8,
		DO_FLASH_WRITE=9,
	};

	static const int BULK_TIMEOUT=500;

	void write( const char *buf, size_t len, int timeout=BULK_TIMEOUT );
	void read( char *buf, size_t len, int timeout=BULK_TIMEOUT );

	void readVersion();

	void readSector( uint16_t adr, char *buf, size_t len ) {
		readMem( pffs_top_+adr*0x1000, buf, len );
	}

	bool doWriteFlash( uint32_t rom_adr, uint32_t buf_adr, size_t len );
public:
	Device();
	~Device();

	void readMem( uint32_t begin, char *buf, size_t len );
	void writeMem( uint32_t begin, const char *buf, size_t len );

	uint32_t getMasterBlockAdr( ) { return mblk_adr_; }
	uint32_t getPffsTop() { return pffs_top_; }

	bool writeFlash( uint32_t rom_adr, const char *buf, size_t buflen, bool safe=true );
	bool eraseFlash( uint32_t adr );

	enum {
		APP_RUN=0x01,
		APP_STOP=0x03,
	};

	void setAppStat( int stat );
	int getAppStat();

	void dumpVersion();
};

}
}

#endif
