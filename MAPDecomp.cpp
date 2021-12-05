#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERROR(message,arg) printf(message, arg); exit(1)

#define INT_SIZE 4 /*sizeof(unsigned int)*/
#define CHR_SIZE 1 /*sizeof(unsigned char)*/
#define SHR_SIZE 2 /*sizeof(unsigned short)*/
#define FLT_SIZE 4 /*sizeof(float)*/
#define PTR_SIZE sizeof(void*)
#define FILENAMES_BUFFER_SIZE 65535
#define FILENAME_SIZE 256

void mkdirtofile(char* path) {
	size_t length = strlen(path);
	char* token;
	char* nextToken = {};
	//strtok replaces the delimiter with \0
	token = strtok_s(path, "\\/", &nextToken);
	while (strlen(path) != length) {
		_mkdir(path);
		//put the delimiter back
		*(nextToken-1) = '\\';
		token = strtok_s(NULL, "\\/", &nextToken);
	}
}

void writeBuffer(const void *_Str, size_t _Size, size_t _Count, const char *_Path, const char *_Mode) {
	FILE* outFile;
	fopen_s(&outFile, _Path, _Mode);
	if (NULL == outFile) { ERROR("Could not open \"%s\" for writing.\n", _Path); }
	if (fwrite(_Str, _Size, _Count, outFile) != _Count) { ERROR("Failure writing \"%s\".\n", _Path); }
	fflush(outFile);
	fclose(outFile);
}

/*
bool fileExists(char* path) {
	bool result = false;
	FILE* file;
	errno_t err = fopen_s(&file, path, "r");
	if (0 == err) {
		result = true;
		fclose(file);
	}
	return result;
}
*/

void main(int argc, char **argv) {
	if (3 != argc) { ERROR("Usage:\n%s Input.map Output.wrl\n", argv[0]); }
	FILE* mapFile;
	FILE* wrlFile;
	unsigned int sec[2] = { 0,0 };
	int eofdetect = 0;

	//Open input file
	fopen_s(&mapFile, argv[1], "rb");
	if (NULL == mapFile) { ERROR("Could not open \"%s\" for reading.\n", argv[1]); }
	fread(sec, INT_SIZE, 2, mapFile);
	if (sec[0] != 0x0650414D) { ERROR("\n\"%s\" is not a map.\n", argv[1]); }

	//Open world file
	fopen_s(&wrlFile, argv[2], "wb");
	if (NULL == wrlFile) { ERROR("Could not open \"%s\" for writing.\n", argv[2]); }
	sec[0] = 0x074C5257;
	fwrite(sec, INT_SIZE, 1, wrlFile);

	bool outOfOrder = true;

	char ** fileNames = (char **) malloc(FILENAMES_BUFFER_SIZE * sizeof(PTR_SIZE));

	for (int i = 0; i < FILENAMES_BUFFER_SIZE; i++) {
		fileNames[i] = (char *) malloc(FILENAME_SIZE);
	}

	eofdetect = fgetc(mapFile);
	unsigned char* buffer;
	while (!feof(mapFile) && !ferror(mapFile)) {
		ungetc(eofdetect, mapFile);
		fread(sec, INT_SIZE, 2, mapFile);
		printf("0x%.8X (%010i) - 0x%.8X (%010i): Section %.2X\n", ftell(mapFile), ftell(mapFile), sec[1], sec[1], sec[0]);


		buffer = (unsigned char*)malloc(CHR_SIZE * sec[1]);

		//Data chunks. Parsed and output by other sections.
		if (0x02 == sec[0] || 0x1A == sec[0]) {
			if (fseek(mapFile, sec[1], SEEK_CUR) != 0) { ERROR("Failure reading \"%s\".\n", argv[1]); }
		}
		else {
			if (NULL == buffer) { ERROR("%s\n", "Out of memory."); }
			if (fread(buffer, CHR_SIZE, sec[1], mapFile) != sec[1]) { ERROR("Failure reading \"%s\".\n", argv[1]); }
		}

		//Levels
		if (0x01 == sec[0]) {
			outOfOrder = false;

			long pos = ftell(mapFile);

			int count = ((int*)buffer)[0];
			int sizes[4] = {0x13, 0, 0, 0};
			sizes[1] = sec[1] - (count*8);
			if (fwrite(sizes,  INT_SIZE, 2, wrlFile) != 2) { ERROR("Failure writing \"%s\".\n", argv[2]); }
			if (fwrite(buffer, INT_SIZE, 1, wrlFile) != 1) { ERROR("Failure writing \"%s\".\n", argv[2]); }

			int ints[56] = {};
			float* floats = (float*)ints;
			for (int i = 0; i < 14*INT_SIZE; i++) ints[i] = 0;

			int index = INT_SIZE;
			char dbr[256];
			for (int i = 0; i < count; i++) {
				//read 13 ints
				memcpy(ints,  buffer+index, 13*INT_SIZE); index += (13*INT_SIZE);
				for (int j = 0; j < 6; j++) floats[j] = (float)ints[j];
				//read the dbr entry
				memcpy(sizes+0,  buffer+index, INT_SIZE); index += INT_SIZE;
				memcpy(dbr,      buffer+index, sizes[0]); index += sizes[0];
				dbr[sizes[0]] = 0;
				//read the level file name
				memcpy(sizes+1,  buffer+index, INT_SIZE); index += INT_SIZE;
				memcpy(fileNames[i], buffer+index, sizes[1]); index += sizes[1];
				fileNames[i][sizes[1]] = 0;
				//read the offset and length
				memcpy(sizes+2,  buffer+index, INT_SIZE); index += INT_SIZE;
				memcpy(sizes+3,  buffer+index, INT_SIZE); index += INT_SIZE;
				printf("0x%.8X (%010i) - 0x%.8X (%010i): %s\n", sizes[2], sizes[2], sizes[3], sizes[3], fileNames[i]);

				//write the world
				if (fwrite(sizes+1,      INT_SIZE, 1,        wrlFile) != (size_t)1)        { ERROR("Failure writing \"%s\".\n", argv[2]); }
				if (fwrite(fileNames[i], CHR_SIZE, sizes[1], wrlFile) != (size_t)sizes[1]) { ERROR("Failure writing \"%s\".\n", argv[2]); }
				if (fwrite(ints,         FLT_SIZE, 13,       wrlFile) != (size_t)13)       { ERROR("Failure writing \"%s\".\n", argv[2]); }
				if (fwrite(sizes+0,      INT_SIZE, 1,        wrlFile) != (size_t)1)        { ERROR("Failure writing \"%s\".\n", argv[2]); }
				if (fwrite(dbr,          CHR_SIZE, sizes[0], wrlFile) != (size_t)sizes[0]) { ERROR("Failure writing \"%s\".\n", argv[2]); }
				fflush(wrlFile);

				mkdirtofile(fileNames[i]);

				//read the rlv data
				fileNames[i][sizes[1]-3] = 'r';
				fileNames[i][sizes[1]-2] = 'l';
				fileNames[i][sizes[1]-1] = 'v';
				unsigned char* lvl = (unsigned char*) malloc(CHR_SIZE*sizes[3]);
				if (NULL == lvl) { ERROR("%s\n", "Out of memory."); }
				if (fseek(mapFile, sizes[2], SEEK_SET) != 0) { ERROR("Failure reading \"%s\".\n", argv[1]); }
				if (fread(lvl, CHR_SIZE, sizes[3], mapFile) != (size_t)sizes[3]) { ERROR("Failure reading \"%s\".\n", argv[1]); }
				//write the rlv data
				writeBuffer(lvl, CHR_SIZE, sizes[3], fileNames[i], "wb");

				//Construct the lvl data
				fileNames[i][sizes[1]-3] = 'l';
				fileNames[i][sizes[1]-2] = 'v';
				fileNames[i][sizes[1]-1] = 'l';

				int lvlIndex = 0;
				unsigned int lvlsec[8];
				memcpy(lvlsec,  lvl+lvlIndex, INT_SIZE); lvlIndex += INT_SIZE;
				writeBuffer(lvlsec, INT_SIZE, 1, fileNames[i], "wb");

				while (lvlIndex < sizes[3]) {
					memcpy(lvlsec,  lvl+lvlIndex, INT_SIZE*7); lvlIndex += INT_SIZE*2;
					if (0x06 == lvlsec[0] && 0x02 == lvlsec[2] && 0x01 == lvlsec[3]) {

						//calculate sizes
						int rlvSize = lvlsec[1];
						int dbrCount = lvlsec[4];
						int dbrWord = (dbrCount/8)+1;
						int x = lvlsec[5];
						int y = lvlsec[6];

						lvlIndex += INT_SIZE*9;

						unsigned char* block1 = lvl+lvlIndex;
						int block1Size = x*y*FLT_SIZE;
						lvlIndex += block1Size;

						int block2Size = x*y*dbrWord;
						lvlIndex += block2Size;

						unsigned char* block3 = lvl+lvlIndex;
						int block3Size = 0;
						int dbrSize = 0;
						memcpy(&dbrSize, lvl+lvlIndex, INT_SIZE);
						lvlIndex += dbrSize + INT_SIZE;
						block3Size += dbrSize + INT_SIZE;
						for (int j = 1; j < dbrCount; j++) {
							memcpy(&dbrSize, lvl+lvlIndex, INT_SIZE);
							lvlIndex += dbrSize + INT_SIZE;
							block3Size += dbrSize + INT_SIZE;
							lvlIndex += (x-1)*(y-1);
							block3Size += (x-1)*(y-1);
						}

						int block4Size = rlvSize - (INT_SIZE*9) - block1Size - block2Size - block3Size;
						lvlIndex += block4Size;

						int lvlSize = (4*INT_SIZE) + block3Size + (block1Size*2) + (block1Size*3) + ((x-1)*(y-1)*INT_SIZE);

						//new header
						lvlsec[1] = lvlSize;
						lvlsec[2] = 0;
						lvlsec[3] = x;
						lvlsec[4] = y;
						lvlsec[5] = dbrCount;
						writeBuffer(lvlsec, INT_SIZE, 6, fileNames[i], "ab");

						//skip to the dbrs
						writeBuffer(block3, CHR_SIZE, block3Size, fileNames[i], "ab");

						int block = block1Size/FLT_SIZE;
						char* data;
						data = (char*)calloc(block, FLT_SIZE*2);
						if (NULL == data) { ERROR("%s\n", "Out of memory."); }
						for (int j = 0; j < block; j++)
							memcpy(data+(FLT_SIZE*2*j), block1+(FLT_SIZE*j), FLT_SIZE);
						writeBuffer(data, FLT_SIZE*2, block, fileNames[i], "ab");
						free(data);
						data = (char*)calloc(block, FLT_SIZE*3);
						if (NULL == data) { ERROR("%s\n", "Out of memory."); }
						for (int j = 0; j < block; j++)
							memcpy(data+(FLT_SIZE*3*j), block1+(FLT_SIZE*j), FLT_SIZE);
						writeBuffer(data, FLT_SIZE*3, block, fileNames[i], "ab");
						free(data);

						data = (char*)calloc(block, INT_SIZE);
						if (NULL == data) { ERROR("%s\n", "Out of memory."); }
						//WTF goes here?! Zeroes unless anyone finds otherwise....
						writeBuffer(data, INT_SIZE, (x-1)*(y-1), fileNames[i], "ab");
						free(data);
					}
					else {
						writeBuffer(lvlsec, INT_SIZE, 2, fileNames[i], "ab");
						writeBuffer(lvl+lvlIndex, CHR_SIZE, lvlsec[1], fileNames[i], "ab");
						lvlIndex += lvlsec[1];
					}

				}

				free(lvl);
			}

			fseek(mapFile, pos, SEEK_SET);
		}
		//Quests, groups
		else if (0x1B == sec[0] || 0x11 == sec[0]) {
			if (fwrite(sec, INT_SIZE, 2, wrlFile) != 2) { ERROR("Failure writing \"%s\".\n", argv[2]); }
			if (fwrite(buffer, CHR_SIZE, sec[1], wrlFile) != sec[1]) { ERROR("Failure writing \"%s\".\n", argv[2]); }
			fflush(wrlFile);
		}
		//Bitmaps
		else if (0x19 == sec[0] && !outOfOrder) {
			long pos = ftell(mapFile);

			int* ints = (int*)buffer;
			int tgaCount = ints[1];
			unsigned char** tga = (unsigned char**)malloc(tgaCount*PTR_SIZE);
			int* tgaSize = (int*)malloc(tgaCount*INT_SIZE);
			
			int offset, length;
			for (int i = 0; i < tgaCount; i++) {
				*(fileNames[i]+strlen(fileNames[i])-3) = 't';
				*(fileNames[i]+strlen(fileNames[i])-2) = 'g';
				*(fileNames[i]+strlen(fileNames[i])-1) = 'a';
				offset = ints[(2*i)+2];
				length = ints[(2*i)+3];
				printf("0x%.8X (%010i) - 0x%.8X (%010i): %s\n", offset, offset, length, length, fileNames[i]);

				//read the data
				tga[i] = (unsigned char*) malloc(length);
				if (NULL == tga[i]) { ERROR("%s\n", "Out of memory."); }
				if (fseek(mapFile, offset, SEEK_SET) != 0) { ERROR("Failure reading \"%s\".\n", argv[1]); }
				if (fread(tga[i], CHR_SIZE, length, mapFile) != (size_t)length) { ERROR("Failure reading \"%s\".\n", argv[1]); }

				//write the data
				writeBuffer(tga[i], CHR_SIZE, length, fileNames[i], "wb");

				//shrink the data
				if (length > 0) {
					short* tgaHeader = (short*)tga[i];
					short oldX = tgaHeader[6];
					short oldY = tgaHeader[7];
					short newX = oldX/4;
					short newY = oldY/4;
					tgaSize[i] = (newX*newY*3)+0x12;

					for (short x = 0; x < newX; x++) {
						for (short y = 0; y < newY; y++) {
							tga[i][y*newX*3 + x*3 + 0x12] = tga[i][(y*oldX*4)*3 + (x*4)*3 + 0x12];
							tga[i][y*newX*3 + x*3 + 0x13] = tga[i][(y*oldX*4)*3 + (x*4)*3 + 0x13];
							tga[i][y*newX*3 + x*3 + 0x14] = tga[i][(y*oldX*4)*3 + (x*4)*3 + 0x14];
						}
					}

					tgaHeader[6] = newX;
					tgaHeader[7] = newY;
					tga[i] = (unsigned char*)realloc(tga[i], tgaSize[i]);
				}
				else tgaSize[i] = 0;

			}

			//done with buffer/ints, repurposing
			ints[0] = 0x15;
			ints[1] = 0;
			for (int i = 0; i < tgaCount; i++) ints[1] += tgaSize[i] + (4*INT_SIZE);
			if (fwrite(ints, INT_SIZE, 2, wrlFile) != 2) { ERROR("Failure writing \"%s\".\n", argv[2]); }
			for (int i = 0; i < tgaCount; i++) {
				short* tgaHeader = (short*)tga[i];
				ints[0] = 0;
				ints[1] = tgaHeader[6];
				ints[2] = tgaHeader[7];
				ints[3] = tgaSize[i];
				if (fwrite(ints, INT_SIZE, 4, wrlFile) != 4) { ERROR("Failure writing \"%s\".\n", argv[2]); }
				if (fwrite(tga[i], CHR_SIZE, tgaSize[i], wrlFile) != (size_t)tgaSize[i]) { ERROR("Failure writing \"%s\".\n", argv[2]); }

				free(tga[i]);
			}
			free(tga);
			free(tgaSize);

			fseek(mapFile, pos, SEEK_SET);
		}
		//.sd
		else if (0x18 == sec[0]) {
			sprintf_s(fileNames[FILENAMES_BUFFER_SIZE-1], 255, "%s", argv[2]);
			*(fileNames[FILENAMES_BUFFER_SIZE-1]+strlen(fileNames[FILENAMES_BUFFER_SIZE-1])-3) = 's';
			*(fileNames[FILENAMES_BUFFER_SIZE-1]+strlen(fileNames[FILENAMES_BUFFER_SIZE-1])-2) = 'd';
			*(fileNames[FILENAMES_BUFFER_SIZE-1]+strlen(fileNames[FILENAMES_BUFFER_SIZE-1])-1) = 0;

			writeBuffer(buffer, CHR_SIZE, sec[1], fileNames[FILENAMES_BUFFER_SIZE-1], "wb");
		}

		//Data chunks. Output by other sections.
/*		if (0x02 != sec[0] && 0x1A != sec[0]) {
			sprintf_s(fileNames[1023], 255, "Section%.2X.hex", sec[0]);

			if (!fileExists(fileNames[1023])) {
				appendBuffer(sec, INT_SIZE, 2, fileNames[1023]);
				appendBuffer(buffer, CHR_SIZE, sec[1], fileNames[1023]);
			}
		}
*/
		free(buffer);

		eofdetect = fgetc(mapFile);
	}


	for (int i = 0; i < FILENAMES_BUFFER_SIZE; i++) {
		free(fileNames[i]);
	}


	//close the world. open the nExt.
	fflush(wrlFile);
	fclose(wrlFile);

	exit(0);
}