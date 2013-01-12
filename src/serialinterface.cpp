//
// Configuration
//

// Header
#include "serialinterface.h"

// Standard library
#include <string.h>

// Platform
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/file.h>

// Local includes
#include "auxiliary.h"


//
// Construction and destruction
//

SerialInterface::SerialInterface(const std::string& portname)
{
    // Open the port
    clog(info) << "open_weatherstation" << std::endl;
    if ((_sp = open(portname.c_str(), O_RDWR | O_NOCTTY)) < 0)
        throw HardwareException("Unable to open serial device");
    if ( flock(_sp, LOCK_EX) < 0 )
        throw HardwareException("Serial device is locked by other program");

    // We want full control of what is set by simply resetting entire adtio
    // struct    
    struct termios adtio;
    memset(&adtio, 0, sizeof(adtio));

    // Serial control options
    adtio.c_cflag &= ~PARENB;      // No parity
    adtio.c_cflag &= ~CSTOPB;      // One stop bit
    adtio.c_cflag &= ~CSIZE;       // Character size mask
    adtio.c_cflag |= CS8;          // Character size 8 bits
    adtio.c_cflag |= CREAD;        // Enable Receiver
    //adtio.c_cflag &= ~CREAD;        // Disable Receiver
    adtio.c_cflag &= ~HUPCL;       // No "hangup"
    adtio.c_cflag &= ~CRTSCTS;     // No flowcontrol
    adtio.c_cflag |= CLOCAL;       // Ignore modem control lines

    // Baudrate, for newer systems
    cfsetispeed(&adtio, BAUDRATE);
    cfsetospeed(&adtio, BAUDRATE);

    // Local options
    //   Raw input = clear ICANON, ECHO, ECHOE, and ISIG
    //   Disable misc other local features = clear FLUSHO, NOFLSH, TOSTOP, PENDIN, and IEXTEN
    // So we actually clear all flags in adtio.c_lflag
    adtio.c_lflag = 0;

    // Input options
    //   Disable parity check = clear INPCK, PARMRK, and ISTRIP
    //   Disable software flow control = clear IXON, IXOFF, and IXANY
    //   Disable any translation of CR and LF = clear INLCR, IGNCR, and ICRNL
    //   Ignore break condition on input = set IGNBRK
    //   Ignore parity errors just in case = set IGNPAR;
    // So we can clear all flags except IGNBRK and IGNPAR
    adtio.c_iflag = IGNBRK|IGNPAR;

    // Output options
    // Raw output should disable all other output options
    adtio.c_oflag &= ~OPOST;

    // Time-out options
    adtio.c_cc[VTIME] = 10;     // timer 1s
    adtio.c_cc[VMIN] = 0;       // blocking read until 1 char

    if (tcsetattr(_sp, TCSANOW, &adtio) < 0)
        throw HardwareException("Unable to initialize serial device");
    tcflush(_sp, TCIOFLUSH);
}

SerialInterface::~SerialInterface()
{
    tcflush(_sp, TCIOFLUSH);
    close(_sp);
}


//
// Low-level port interface
// 

void SerialInterface::set_DTR(bool value)
{
    // TODO: use TIOCMBIC and TIOCMBIS instead of TIOCMGET and TIOCMSET
    int portstatus;
    ioctl(_sp, TIOCMGET, &portstatus);   // get current port status
    if (value)
    {
        clog(trace) << "Set DTR" << std::endl;
        portstatus |= TIOCM_DTR;
    }
    else
    {
        clog(trace) << "Clear DTR" << std::endl;
        portstatus &= ~TIOCM_DTR;
    }
    ioctl(_sp, TIOCMSET, &portstatus);   // set current port status

    /*if (value)
      ioctl(_sp, TIOCMBIS, TIOCM_DTR);
    else
      ioctl(_sp, TIOCMBIC, TIOCM_DTR);*/
}

void SerialInterface::set_RTS(bool value)
{
    //TODO: use TIOCMBIC and TIOCMBIS instead of TIOCMGET and TIOCMSET
    int portstatus;
    ioctl(_sp, TIOCMGET, &portstatus);   // get current port status
    if (value)
    {
        clog(trace) << "Set RTS" << std::endl;
        portstatus |= TIOCM_RTS;
    }
    else
    {
        clog(trace) << "Clear RTS" << std::endl;
        portstatus &= ~TIOCM_RTS;
    }
    ioctl(_sp, TIOCMSET, &portstatus);   // set current port status

    /*if (value)
      ioctl(_sp, TIOCMBIS, TIOCM_RTS);
    else
      ioctl(_sp, TIOCMBIC, TIOCM_RTS);
    */

}

bool SerialInterface::get_DSR()
{
    int portstatus;
    ioctl(_sp, TIOCMGET, &portstatus);   // get current port status

    if (portstatus & TIOCM_DSR)
    {
        clog(trace) << "Got DSR = 1" << std::endl;
        return true;
    }
    else
    {
        clog(trace) << "Got DSR = 0" << std::endl;
        return false;
    }
}

bool SerialInterface::get_CTS()
{
    int portstatus;
    ioctl(_sp, TIOCMGET, &portstatus);   // get current port status

    if (portstatus & TIOCM_CTS)
    {
        clog(trace) << "Got CTS = 1" << std::endl;
        return true;
    }
    else
    {
        clog(trace) << "Got CTS = 0" << std::endl;
        return false;
    }
}

int SerialInterface::read_device(unsigned char *data, size_t length)
{
    int ret;

    for (;;) {
        ret = read(_sp, data, length);
        if (ret == 0 && errno == EINTR)
            continue;
        return ret;
    }
}

int SerialInterface::write_device(const unsigned char *data, size_t length)
{
    int ret = write(_sp, data, length);
    return ret;
}


//
// Bit-level I/O operations
// 

byte SerialInterface::read_bit()
{
    clog(trace) << "Read bit ..." << std::endl;
    set_DTR(false);
    nanodelay();
    bool status = get_CTS();
    nanodelay();
    set_DTR(true);
    nanodelay();
    clog(trace) << "Bit = " << (status ? "0" : "1") << std::endl;

    return (byte)(status ? 0 : 1);
}

void SerialInterface::write_bit(bool bit)
{
    clog(trace) << "Write bit " << (bit ? "1" : "0") << std::endl;
    set_RTS(!bit);
    nanodelay();
    set_DTR(false);
    nanodelay();
    set_DTR(true);
}


//
// Byte-level I/O operations
// 

byte SerialInterface::read_byte()
{
    clog(trace) << "Read byte ..." << std::endl;
    byte b = 0;
    for (size_t i = 0; i < 8; i++)
    {
        b *= 2;
        b += read_bit();
    }
    clog(trace) << "byte = 0x" << std::hex << b << std::endl;
    return b;
}

bool SerialInterface::write_byte(byte value, bool verify)
{
    clog(trace) << "Send byte 0x" << std::hex << value << std::endl;

    for (size_t i = 0; i < 8; i++)
    {
        write_bit((value & 0x80) > 0);
        value <<= 1;
    }
    set_RTS(false);
    nanodelay();
    bool status = true;
    if (verify)
    {
        status = get_CTS();
        //TODO: checking value of status, error routine
        nanodelay();
        set_DTR(false);
        nanodelay();
        set_DTR(true);
        nanodelay();
    }
    return status;
}

void SerialInterface::read_next()
{
    clog(debug) << "read_next_byte_seq" << std::endl;
    set_RTS(true);
    nanodelay();
    set_DTR(false);
    nanodelay();
    set_DTR(true);
    nanodelay();
    set_RTS(false);
    nanodelay();
}


//
// Generic I/O operations
//

std::vector<byte> SerialInterface::read_data(address location, size_t length)
{
    if (!query_address(location) || !send_command(0xA1))
        return std::vector<byte>(); // TODO: error?

    std::vector<byte> readdata(length);
    readdata[0] = read_byte();
    for (size_t i = 1; i < length; i++)
    {
        read_next();
        readdata[i] = read_byte();
    }
    end_command();

    return readdata;
}

bool SerialInterface::write_data(address location, const std::vector<byte> &data)
{
    start_sequence();
    if (!query_address(location))
        return false;
    for (size_t i = 0; i < data.size(); i++)
        if (!write_byte(data[i]))
            return false;
    end_command();

    start_sequence();
    for (size_t i = 0; i < 3; i++) 
        send_command(0xA0, false);

    set_DTR(false);
    nanodelay();
    bool result = get_CTS();
    set_DTR(true);
    nanodelay();

    return result;
}


//
// Command interface
//

bool SerialInterface::send_command(byte command, bool verify)
{
    set_DTR(false);
    nanodelay();
    set_RTS(false);
    nanodelay();
    set_RTS(true);
    nanodelay();
    set_DTR(true);
    nanodelay();
    set_RTS(false);
    nanodelay();

    return write_byte(command, verify);
}

void SerialInterface::start_sequence()
{
    clog(trace) << "start_sequence" << std::endl;
    set_RTS(false);
    nanodelay();
    set_DTR(false);
    nanodelay();
}

void SerialInterface::end_command()
{
    clog(trace) << "end_command" << std::endl;
    set_RTS(true);
    nanodelay();
    set_DTR(false);
    nanodelay();
    set_RTS(false);
    nanodelay();
}


//
// Auxiliary
// 

void SerialInterface::nanodelay()
{
    usleep(4);
}

bool SerialInterface::query_address(address location)
{
    return send_command(0xA0)
        && write_byte((uint8_t)(location / 256))
        && write_byte((uint8_t)(location % 256));
}
