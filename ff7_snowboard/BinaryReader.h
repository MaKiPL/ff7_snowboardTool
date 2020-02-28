#pragma once
#include <fstream>
class BinaryReader
{
public:
	BinaryReader();
	BinaryReader(std::string filePath);
	bool bIsOpened = false;
	void ReadBuffer(char* c, int count);
	short ReadInt16();
	unsigned long ReadUInt32();
	int ReadInt32();
	unsigned char ReadByte();
	void seek(int offset, int mode);
	int tell();

private:
	std::ifstream fd;
};

