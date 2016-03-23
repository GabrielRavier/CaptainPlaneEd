#include <SDL2/SDL.h>

#include "PrjHndl.h"
#include "TxtRead.h"
#include "Resource.h"

#include "compression/KidDec.h"
#include "compression/ReadPlain.h"
#include "FW_KENSC/comper.h"
#include "FW_KENSC/enigma.h"
#include "FW_KENSC/kosinski.h"
#include "FW_KENSC/nemesis.h"
#include "FW_KENSC/saxman.h"

const char* const FILE_MAP_DEFAULT = "MapDefault.bin";

Resource::Resource(void)
{
	strcpy(this->name, "");
	this->offset = 0;
	this->length = 0;
	this->compression = comprType::INVALID;
	this->kosinski_module_size = 0x1000;
}

void Resource::Save(const char* const filename, const char* const dstfilename)
{
	CompressFile(filename, dstfilename);
	remove(filename);
}

long Resource::DecompressToFile(const char* const dstfile)
{
	int decompressed_length;
	switch (this->compression)
	{
		case comprType::NONE:
			decompressed_length = ReadPlain(this->name, dstfile, this->offset, this->length);
			break;
		case comprType::ENIGMA:
			decompressed_length = enigma::decode(this->name, dstfile, this->offset, false);
			break;
		case comprType::KOSINSKI:
			decompressed_length = kosinski::decode(this->name, dstfile, this->offset, false, 16u);
			break;
		case comprType::MODULED_KOSINSKI:
			decompressed_length = kosinski::decode(this->name, dstfile, this->offset, true, 16u);
			break;
		case comprType::NEMESIS:
			decompressed_length = nemesis::decode(this->name, dstfile, this->offset, 0);
			break;
		case comprType::KID_CHAMELEON:
			decompressed_length = KidDec(this->name, dstfile, this->offset);
			break;
		case comprType::COMPER:
			decompressed_length = comper::decode(this->name, dstfile, this->offset);
			break;
		case comprType::SAXMAN:
			decompressed_length = saxman::decode(this->name, dstfile, this->offset, 0);
			break;
	}

	return decompressed_length;
}

void Resource::CompressFile(const char* const srcfile, const char* const dstfile)
{
	switch (this->compression)
	{
		case comprType::NONE:
			remove(dstfile);
			rename(srcfile, dstfile);
			break;
		case comprType::ENIGMA:
			enigma::encode(srcfile, dstfile, false);
			break;
		case comprType::KOSINSKI:
			kosinski::encode(srcfile, dstfile, false, this->kosinski_module_size, 16u);
			break;
		case comprType::MODULED_KOSINSKI:
			kosinski::encode(srcfile, dstfile, true, this->kosinski_module_size, 16u);
			break;
		case comprType::NEMESIS:
			nemesis::encode(srcfile, dstfile);
			break;
		case comprType::COMPER:
			comper::encode(srcfile, dstfile);
			break;
		case comprType::SAXMAN:
			saxman::encode(srcfile, dstfile, false);
			break;
	}
}

ResourceArt::ResourceArt(void)
{
	this->tileAmount = 0;
}

void ResourceArt::Load(const char* const filename)
{
	if (this->compression == comprType::INVALID)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Invalid art compression format. Should be one of the following:\n\n'None'\n'Enigma'\n'Kosinski'\n'Moduled Kosinski'\n'Nemesis'\n'Kid Chameleon'\n'Comper'\n'Saxman'", NULL);
		exit(1);
	}

	int decompressed_length = DecompressToFile(filename);

	if (decompressed_length < 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not decompress art file. Are you sure the compression is correct?", NULL);
		exit(1);
	}
	this->tileAmount = decompressed_length/0x20;
}

ResourceMap::ResourceMap(void)
{
	this->xSize = 0;
	this->ySize = 0;
	strcpy(this->saveName, "");
}

void ResourceMap::Load(const char* const filename)
{
	if (this->compression == comprType::INVALID || this->compression == comprType::KID_CHAMELEON)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Invalid map compression format. Should be one of the following:\n\n'None'\n'Enigma'\n'Kosinski'\n'Moduled Kosinski'\n'Nemesis'\n'Comper'\n'Saxman'", NULL);
		exit(1);
	}

	int decompressed_length = DecompressToFile(filename);

	if (decompressed_length < 0) {
		//file could not be decompressed or found
		decompressed_length = 2*this->xSize*this->ySize;
		if (!CheckCreateBlankFile(this->name, filename, this->offset, decompressed_length))
		{
			//file is existant but could not be decompressed
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not decompress map file. Are you sure the compression is correct?", NULL);
			exit(1);
		}
		else
		{
			//file non-existant, blank template created
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Information", "No map file found, created blank template.", NULL);
		}
	}

	if (decompressed_length < 2*this->xSize*this->ySize)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Warning", "Specified size exceeds map size.\nField has been trimmed vertically.", NULL);
		this->ySize = (decompressed_length/this->xSize) / 2;
		if (this->ySize == 0)
			exit(1);
	}

	if (strcmp(this->saveName, "") == 0)
	{
		if (this->offset == 0)
		{
			strcpy(this->saveName, this->name); //overwrite existing map
		}
		else
		{
			const char* const part_message = "This tool cannot overwrite a ROM. Plane map will be saved to ";
			char* whole_message = new char[strlen(part_message)+strlen(FILE_MAP_DEFAULT)+1];
			sprintf(whole_message, "%s%s", part_message, FILE_MAP_DEFAULT);
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Information", whole_message, NULL);
			delete[] whole_message;
			strcpy(this->saveName, FILE_MAP_DEFAULT); //write to default file
		}
	}
}

ResourcePal::ResourcePal(void)
{
	// For backwards compatibility, palette is assumed to be uncompressed by default
	this->compression = comprType::NONE;
}

void ResourcePal::Load(const char* const filename)
{
	if (this->compression == comprType::INVALID)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Invalid palette compression format. Should be one of the following:\n\n'None'\n'Enigma'\n'Kosinski'\n'Moduled Kosinski'\n'Nemesis'\n'Kid Chameleon'\n'Comper'\n'Saxman'", NULL);
		exit(1);
	}

	int decompressed_length = DecompressToFile(filename);

	if (decompressed_length < 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", "Could not decompress palette file. Are you sure the compression is correct?", NULL);
		exit(1);
	}
}
