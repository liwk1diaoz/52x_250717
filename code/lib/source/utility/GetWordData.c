#include "BinaryFormat.h"

/**
    Get a 32-bit data.

    Get a 32-bit data from specified address, no matter whether the address is not word-aligned.

    @param[in] Addr              Data Address.
    @param[in] bIsLittleEndian   Byte Order.
                            - @b TRUE: Little endian.
                            - @b FALSE: Big endian.

    @return A 32-bit data.
*/
UINT32 Get32BitsData(UINT32 Addr, BOOL bIsLittleEndian)
{
    UINT32 value = 0;
    UINT8 *pb = (UINT8 *)Addr;
    if(bIsLittleEndian)
    {
        if(Addr & 0x3)//NOT word aligned
        {
            value =  (*pb);
            value |= (*(pb + 1))<<8;
            value |= (*(pb + 2))<<16;
            value |= (*(pb + 3))<<24;
        }
        else
        {
            value = *(UINT32 *)Addr;
        }
    }
    else
    {
        value =  (*pb)<<24;
        value |= (*(pb + 1))<<16;
        value |= (*(pb + 2))<<8;
        value |= (*(pb + 3));
    }
    return value;
}

/**
    Get a 16-bit data.

    Get a 16-bit data from specified address, no matter whether the address is not half-word-aligned.

    @param[in] Addr              Data Address.
    @param[in] bIsLittleEndian   Byte Order.
                            - @b TRUE: Little endian.
                            - @b FALSE: Big endian.

    @return A 16-bit data.
*/
UINT16 Get16BitsData(UINT32 Addr, BOOL bIsLittleEndian)
{
    UINT16 value = 0;
    UINT8 *pb = (UINT8 *)Addr;
    if(bIsLittleEndian)
    {
        value =  (*pb);
        value |= (*(pb + 1))<<8;
    }
    else
    {
        value =  (*pb)<<8;
        value |= (*(pb + 1));
    }
    return value;
}

/**
    Set a 32-bit data.

    Set a 32-bit data to specified address, no matter whether the address is not word-aligned.

    @param[in] Addr              Data address.
    @param[in] Value             Data value.
    @param[in] bIsLittleEndian   Byte Order.
                            - @b TRUE: Little endian.
                            - @b FALSE: Big endian.

    @return void
*/
void Set32BitsData(UINT32 Addr, UINT32 Value, BOOL bIsLittleEndian)
{
    UINT8 *pb = (UINT8 *)Addr;
    if(bIsLittleEndian)
    {
        if(Addr & 0x3)//NOT word aligned
        {
            *(pb) = (Value&0xff);
            *(pb+1) = (Value>>8)&0xFF;
            *(pb+2) = (Value>>16)&0xFF;
            *(pb+3) = (Value>>24)&0xFF;
        }
        else
        {
            *(UINT32 *)Addr = Value;
        }
    }
    else
    {
        *(pb) = (Value>>24);
        *(pb+1) = (Value>>16);
        *(pb+2) = (Value>>8);
        *(pb+3) = (Value);
    }
}

/**
    Set a 16-bit data.

    Set a 16-bit data to specified address, no matter whether the address is not half-word-aligned.

    @param[in] Addr              Data address.
    @param[in] Value             Data value.
    @param[in] bIsLittleEndian   Byte Order.
                            - @b TRUE: Little endian.
                            - @b FALSE: Big endian.

    @return void
*/
void Set16BitsData(UINT32 Addr, UINT16 Value, BOOL bIsLittleEndian)
{
    UINT8 *pb = (UINT8 *)Addr;
    if(bIsLittleEndian)
    {
        *(pb) = (Value&0xff);
        *(pb+1) = (Value>>8)&0xFF;
    }
    else
    {
        *(pb) = (Value>>8)&0xFF;
        *(pb+1) = (Value&0xff);
    }
}

/**
    Gets a 8-bit data.

    Gets a 8-bit data from the specified address and then the address will point to next byte.

    @param[in,out] addr The address of data address.
    @return A 8-bit data.
*/
UINT8 GetCurByte(UINT8 **addr)
{
    UINT8 getdata;

    getdata = **addr;
    *addr += 1;
    return(getdata);
}

/**
    Gets a 16-bit data.

    Gets a 16-bit data from the specified address and then the address will point to next 16-bit data.

    @param[in,out] buf_p The address of data address.
    @param[in] Swap      Byte Order.
                            - @b TRUE: Little endian.
                            - @b FALSE: Big endian.
    @return A 16-bit data.
*/
UINT16 GetCurSHORT(UINT8 **buf_p, BOOL Swap)
{
    UINT16  data;
    if(Swap==0)
    {
        data = (GetCurByte(buf_p)<<8 );
        data |= GetCurByte(buf_p);
    }
    else //if(Swap==0)
    {
        data = GetCurByte(buf_p);
        data |= (GetCurByte(buf_p)<<8 );
    }
    return data;
}

/**
    Gets a 32-bit data.

    Gets a word data from the specified address and then the address will point to next word data.

    @param[in,out] buf_p The address of data address.
    @param[in] Swap      Byte Order.
                            - @b TRUE: Little endian.
                            - @b FALSE: Big endian.
    @return A word data.
*/
UINT32 GetCurUINT(UINT8 **buf_p, BOOL Swap)
{
    UINT32  data;
    if(Swap==0)
    {
        data = (GetCurByte(buf_p)<<24 );
        data |= (GetCurByte(buf_p)<<16 );
        data |= (GetCurByte(buf_p)<<8 );
        data |= GetCurByte(buf_p);
    }
    else //if(Swap==1)
    {
        data = GetCurByte(buf_p);
        data |= (GetCurByte(buf_p)<<8 );
        data |= (GetCurByte(buf_p)<<16 );
        data |= (GetCurByte(buf_p)<<24 );
    }
    return data;
}




