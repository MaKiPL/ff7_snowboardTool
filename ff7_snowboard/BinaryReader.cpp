#include "BinaryReader.h"
#include <string>
#include <fstream>

BinaryReader::BinaryReader()
{
	;
}
BinaryReader::BinaryReader(std::string filePath)
{
	fd = std::ifstream(filePath, std::ios::binary | std::ios::in);
	bIsOpened = true;
}

void BinaryReader::ReadBuffer(char* c, int count)
{
	fd.read(c, count);
}

unsigned char BinaryReader::ReadByte()
{
	char local[1];
	fd.read(local, 1);
	return local[0];
}

short BinaryReader::ReadInt16()
{
	char local[2];
	fd.read(local, 2);
	return *(short*)local;
}

unsigned long BinaryReader::ReadUInt32()
{
	char local[4];
	fd.read(local, 4);
	return *(unsigned long*)local;
}

int BinaryReader::ReadInt32()
{
	char local[4];
	fd.read(local, 4);
	return *(int*)local;
}

void BinaryReader::seek(int offset, int mode)
{
	fd.seekg(offset, mode);
}

int BinaryReader::tell()
{
	return fd.tellg();
}